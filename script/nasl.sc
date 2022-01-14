#nasl 脚本适配转接层，这里实现了许多原nasl里面c的实现函数
#通过这个转接层，减少脚本对c的依赖，去掉c里面许多业务相关的逻辑

var NULL = nil;
var TRUE = true;
var FALSE = false;

var IPPROTO_TCP = 6;
var IPPROTO_UDP = 17;
var IPPROTO_ICMP = 1;
var IPPROTO_ICMPV6 = 58;
var IPPROTO_IP = 0;
var IPPROTO_IGMP = 2;

var ENCAPS_AUTO = 0;
var ENCAPS_IP   = 1;
var ENCAPS_SSLv23 = 2;
var ENCAPS_SSLv2 =3;
var ENCAPS_SSLv3 =4;
var ENCAPS_TLSv1 =5;
var ENCAPS_TLSv11 =6;
var ENCAPS_TLSv12 =7;
var ENCAPS_TLSv13 =8;
var ENCAPS_TLScustom =9;
var ENCAPS_MAX = 10;

var TH_FIN =  0x1;
var TH_SYN = 0x2;
var TH_RST = 0x4;
var TH_PUSH = 0x08;
var TH_ACK = 0x10;
var TH_URG = 0x20;
var IP_RF = 0x8000;
var IP_DF = 0x4000;
var IP_MF = 0x2000;
var IP_OFFMASK = 0x1FFF;
var TCPOPT_MAXSEG = 2;
var TCPOPT_WINDOW = 3;
var TCPOPT_SACK_PERMITTED = 4;
var TCPOPT_TIMESTAMP = 8;

var ACT_INIT = 0;
var ACT_GATHER_INFO = 3;
var ACT_ATTACK = 4;
var ACT_MIXED_ATTACK = 5;
var ACT_DESTRUCTIVE_ATTACK = 6;
var ACT_DENIAL = 7;
var ACT_SCANNER = 1;
var ACT_SETTINGS = 2;
var ACT_KILL_HOST = 8;
var ACT_FLOOD = 9;
var ACT_END = 10;
var MSG_OOB = 0x1;
var NOERR = 0;
var ETIMEDOUT = 1;
var ECONNRESET = 2;
var EUNREACH = 3;
var EUNKNOWN = 99;
var OPENVAS_VERSION = "21.4.3~dev1~git-36d09619-openvas-21.04";

func __index_nil__(index){
	#Println("convet nil object to NASLArray",index,ShortStack());
	return NASLArray();
}

object NASLArray(hash_table={},vector=[]){
	func __len__(){
		var res = 0;
		for v in hash_table{
			res += len(v);
		}
		return res + len(vector);
	}
	func __get_index__(key){
		if(typeof(key) == "integer"){
			if(key<len(self.vector)){
				return self.vector[key];
			}
			return nil;
		}else{
			var list = self.hash_table[key];
			if(typeof(list)!= "array"){
				self.hash_table[key] = [];
				list = [];
			} 
			if (len(list)==0){
				return nil;
			}
			return list[len(list)-1];
		}
	}

	func __set_index__(key,value){
		if(typeof(key) == "integer"){
			self.vector[key] = value;
			return value;
		}
		var list = self.hash_table[key];
		if(typeof(list) != "array"){
			list = [];
		}
		if(len(list) == 0){
			list = append(list,value);
		}else{
			list[len(list)-1] = value;
		}
		self.hash_table[key] = list;
		return value;
	}

	func __enum_all__(){
		var result = [];
		for k,v in self.vector{
			result = append(result,{"__key__":k,"__value__":v});
		}
		for k,v in self.hash_table{
			for v2 in v{
				result = append(result,{"__key__":k,"__value__":v2});
			}
		}
		return result;
	}

	func add_var_to_list(val){
		if(typeof(val)== "NASLArray" ||typeof(val)== "array" ||typeof(val)== "map"){
			for v in val{
				self.add_var_to_list(v);
			}
			return;
		}
		self.vector = append(self.vector,val);
	}

	func add_var_to_array(index,val){
		self.__set_index__(index,val);
	}
}


#
#由于新版数组不能使用下标来索引，为保持兼容性，返回一个map
func make_list(list...){
	if(len(list)==0){
		return NASLArray();
	}
	var ret = NASLArray();
	for v in list{
		ret.add_var_to_list(v);
	}
	return ret;
}

func nasl_make_list_unique(list...){
	var helper = {};
	for v in list{
		if(typeof(v)=="map" || typeof(v)=="array" || typeof(v) == "NASLArray"){
			for v2 in v{
				helper[v2] = 1;
			}
			continue;
		}
		helper[v] = 1;
	}
	var ret = [];
	for k,v in helper{
		ret = append(ret,k);
	}
	return NASLArray(vector:ret,hash_table:{});
}

func power(x,y){
	for( i =0;i<y;i++){
		x*=x;
	}
	return x;
}

func make_array(list...){
	var ret = NASLArray();
	var size = len(list);
	if(size > 0){
		if(size %2){
			error("make_array: the count of parameters not algin to 2 <count="+ToString(size)+">");
		}
		var ret = NASLArray();
		for(i=0;i<size-1;i+=2){
			ret.add_var_to_array(list[i],list[i+1]);
		}
	}
	return ret;
}

func NASLTypeof(name){
	var result = typeof(name);
	if (result == "map" || result == "NASLArray"){
		return "array";
	}
	return result;
}

func script_tag(name, value){
	return ova_script_tag(name,value);
}

func script_xref(name, value){
	return ova_script_xref(name,value);
}

func safe_checks(){
	return false;
}

func set_kb_item(name, value){
	Println(name,"-->",value);
	return ova_set_kb_item(name,value);
}

func get_kb_list(pattern){
	var temp;
	var ret = NASLArray();
	if(ContainsString(pattern,"*")){
		var keys = kb_get_keys(pattern);
		for v in keys{
			temp = kb_get_list(v);
			if(temp && len(temp)){
				for v2 in temp{
					ret.add_var_to_array(v,v2);
				}
			}
		}
		return ret;
	}
	temp =  kb_get_list(pattern);
	if(temp && len(temp)){
		for v2 in temp{
			ret.add_var_to_array(pattern,v2);
		}
	}
	return ret;
}


func log_message(port,protocol,data,uri,proto){
	if(proto != nil){
		protocol = proto;
	}
	return ova_log_message(port,protocol,data,uri);
}

func security_message(port,protocol,data,uri,proto){
	if(proto != nil){
		protocol = proto;
	}
	return ova_security_message(port,protocol,data,uri);
}

func error_message(port,protocol,data,uri,proto){
	if(proto != nil){
		protocol = proto;
	}
	return ova_error_message(port,protocol,data,uri);
}

func build_http_req(method,item,port,data){
	var header ={"Connection":"close"};
    header["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9";
    header["Accept-Encoding"] = "gzip, deflate";
    header["Accept-Language"] = "zh-CN,zh,en;q=0.9,en;q=0.8";
    header["User-Agent"]="Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36";
	if(ToInteger(port) != 80 && ToInteger(port) != 443){
		header["Host"] = get_host_name()+":"+port;
	}else{
		header["Host"] = get_host_name();
	}
	header["Accept-Charset"] = "iso-8859-1,*,utf-8";
	header["Pragma"] = "no-cache";
	header["Cache-Control"] = "no-cache";
	#header["from-nasl"] = "true";
	if(!item){
		item = "/";
	}
	item = URLPathEscape(item);
	var result = method+ " "+item + " HTTP/1.1\r\n";
	for k,v in header{
		result += k;
		result += ": ";
		result += v;
		result += "\r\n";
	}
	result += "\r\n";
	if(data != nil){
		return result + ToString(data);
	}
	return result;
}

func http_get(item, port){
	return build_http_req("GET",item,port,nil);
}

func http_head(item, port){
	return build_http_req("HEAD",item,port,nil);
}

func http_post(item,port,data){
	return build_http_req("POST",item,port,data);
}

func http_put(item,port,data){
	return build_http_req("PUT",item,port,data);
}


func send(socket,data){
	return ova_send(socket,data);
}

func recv_line(socket,length){
	var ret = ConnReadUntil(socket,"\n",length);
	if(!ret){
		return "";
	}
	return string(ret);
}


func recv(socket,length,timeout,min){
	var ret = ova_recv(socket,length,timeout,min);
	if(typeof(ret)=="bytes"){
		return string(ret);
	}
	return ret;
}

func replace_kb_item(name, value){
	return ova_replace_kb_item(name,value);
}


#script_add_preference(name: "Prefix directory", type: "entry", value: "/etc/apache2/", id: 1)
func script_add_preference(name,type, value, id){
	if(id==nil){
		id = -1;
	}
	return ova_script_add_preference(name,type,value,id);
}

#script_get_preference("Launch IT-Grundschutz (10. EL)", id: 1)
func script_get_preference(name,id=-1){
	return ova_script_get_preference(name,id);
}

func script_get_preference_file_content(name,id=-1){
	return ova_script_get_preference_file_content(name,id);
}

#open_sock_tcp(port, transport: ENCAPS_IP)
func open_sock_tcp(port,buffsz=nil,timeout=nil, transport=nil,priority=0){
	if(!timeout){
		timeout = 15;
	}
	var isTLS = false;
	if(transport == nil){
		 isTLS = get_port_transport(port) > 1;
	}else{
		isTLS = transport > 1;
	}
	#Println(get_host_ip(),port,timeout,isTLS);
	var soc = TCPConnect(get_host_ip(),ToInteger(port),timeout,isTLS);
	if(soc){
		ConnSetReadTimeout(soc,5);
		ConnSetWriteTimeout(soc,5);
	}
	return soc;
}

func socket_get_cert(socket){
	if(!socket){
		return nil;
	}
	var cert = ConnGetPeerCert(socket);
	if(cert){
		return X509Query(cert,"image",0);
	}
	return nil;
}

#match(string: r, pattern: "\x01rlogind: Permission denied*", icase: TRUE)
func match(string,pattern,icase=false){
	return ova_match(string,pattern,icase);
}

#pread(cmd: "nmap", argv: make_list( "nmap",open_sock_tcp(port, transport: ENCAPS_IP)
func pread(cmd,argv){
	error("nasl:pread not implement");
}

var _ssh_session_table = {};


func _get_ssh_port(){
	var port = get_kb_item("Services/ssh");
	if(port){
		return port;
	}
	return 22;
}

func nasl_close(socket){
	var session = _ssh_session_table[socket];
	if(session){
		delete(_ssh_session_table,session);
		delete(_ssh_session_table,socket);
	}
	close(socket);
}

#ssh_connect(socket: soc)
#socket,timeout,ip,keytype,cschiphers,sscriphers
#Value SSHConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm)
func ssh_connect(socket=nil,keytype="",ip=""){
	var newSock;
	if(ip == nil || len(ip) == 0){
		ip = get_host_ip();
	}
	if(socket == nil){
		newSock = TCPConnect(ip,_get_ssh_port(),15,false,false);
		socket = newSock;
	}
	if(socket == nil){
		return nil;
	}
	var session = SSHConnect(socket,15,ip,keytype,"","");
	if(session == nil){
		return nil;
	}
	if(!newSock){
		_ssh_session_table[session] = socket;
		_ssh_session_table[socket] = session;
	}
	return session;
}

func ssh_disconnect(session){
	var socket = _ssh_session_table[session];
	if(socket){
		delete(_ssh_session_table,session);
		delete(_ssh_session_table,socket);
	}
	close(session);
}

func ssh_session_id_from_sock(socket){
	if(socket == nil){
		return nil;
	}
	return _ssh_session_table[socket];
}

func ssh_get_sock(session){
	if(session == nil){
		return nil;
	}
	return _ssh_session_table[session];
}

#//session,username,password,privatekey,passphrase
#ssh_userauth(session, login: NULL, password: NULL, privatekey: NULL, passphrase: NULL)
func ssh_userauth(session,login=nil,password=nil, privatekey=nil, passphrase=nil){
	if (session == nil){
		return -1;
	}
	if (login == nil){
		login =  get_kb_item("Secret/SSH/login");
	}
	if (password == nil){
		password = get_kb_item("Secret/SSH/password");
	}
	if (privatekey == nil){
		privatekey = get_kb_item("Secret/SSH/privatekey");
	}
	if (passphrase == nil){
		privatekey = get_kb_item("Secret/SSH/passphrase");
	}
	return SSHAuth(session,ToString(login),ToString(password),ToString(privatekey),ToString(passphrase));
}

func ssh_login_interactive(session,login=nil){
	if (login == nil){
		login =  get_kb_item("Secret/SSH/login");
	}
	return SSHLoginInteractive(session,ToString(login),"");
}


func ssh_login_interactive_pass(session,login=nil,password=""){
	if (login == nil){
		login =  get_kb_item("Secret/SSH/login");
	}
	return SSHLoginInteractive(session,ToString(login),password);
}

#ssh_request_exec(sess, cmd: cmd, stdout: -1, stderr: -1)
func ssh_request_exec(session, cmd,stdout=0,stderr=0){
	var result = SSHExecute(session,cmd);
	if(result == nil){
		return nil;
	}
	if( (stdout==-1 && stderr == -1)|| stdout == 1 ){
		return result["Stdout"];
	}
	if(stdout == 1 && stderr == 1){
		return result["Stdout"]+result["StdErr"];
	}
	if(stderr == 1){
		return result["StdErr"];
	}
	return result["Stdout"]+result["StdErr"];
}
#ssh_shell_open(sess, pty: TRUE)
func ssh_shell_open(session,pty=1,login=nil){
	if (login == nil){
		login =  get_kb_item("Secret/SSH/login");
	}
	return SSHShellOpen(session,ToString(login),pty);
}

#ssh_shell_write(sess, cmd: user + "\n" + pass + "\n" + "show sysinfo\n\nshow inventory\n")
func ssh_shell_write(session, cmd){
	var size = SSHShellWrite(session,cmd);
	if(size == len(cmd)){
		return 0;
	}
	return -1;
}

func ssh_shell_read(session){
	return SSHShellRead(session);
}

func ssh_shell_close(session){
	close(session);
}

func ssh_get_issue_banner(session,login=nil){
	if (login == nil){
		login =  get_kb_item("Secret/SSH/login");
	}
	return SSHGetIssueBanner(session,ToString(login));
}

func ssh_get_server_banner(session){
	return SSHGetServerBanner(session);
}

func ssh_get_auth_methods(session,login=nil){
	if (login == nil){
		login =  get_kb_item("Secret/SSH/login");
	}
	return SSHGetAuthMethod(session,login);
}

func ssh_get_host_key(session){
	return SSHGetPUBKey(session);
}

func sftp_enabled_check(session){
	return SSHIsSFTPEnabled(session);
}



#open_priv_sock_udp(dport: port, sport: port)
func open_priv_sock_udp(dport, sport){
	error("nasl:open_priv_sock_udp not implement");
}

#mktime(sec: uptime_match[6], min: uptime_match[5], hour: uptime_match[4], mday: uptime_match[3], mon: uptime_match[2], year: uptime_match[1])
func mktime(sec, min, hour, mday, mon, year,isdst=-1){
	return ova_mktime(sec,min,hour,mday,mon,year,isdst);
}

#fwrite(data: " ", file: tmpfile)
func fwrite(data, file){
	error("nasl:fwrite not implement");
}

#socket_negotiate_ssl(socket: soc)
func socket_negotiate_ssl(socket){
	if(soc == nil){
		return -1;
	}
	if(ConnTLSHandshake(soc)){
		return 0;
	}
	return -1;
}

func scanner_add_port(port=-1,proto="tcp"){
	set_kb_item("Ports/"+proto+"/"+port,1);
}

func scanner_status(){
}

#gunzip(data: body)
func gunzip(data){
	return DeflateBytes(data);
}

func raw_string(x...){
	var helper = MakeBytes(4096);
	var i = 0;
	for v in x {
		if (typeof(v) == "string"){
			for j in v{
				helper[i]= j;
				i++;
			}
		}else{
			helper[i] = v;
			i++;
		}
	}
	return string(helper[:i]);
}

func isnull(val){
	var type = typeof(val);
	return (type=="nil" || type == "undef");
}

func defined_func(name){
	return IsFunctionExist(name);
}

func strcat(strlist...){
	var ret = "";
	for v in strlist{
		ret +=v;
	}
	return ret;
}
func ord(obj){
	if(typeof(obj) == "string"){
		if(len(obj)){
			return ToInteger(obj[0]);
		}
		return '0';
	}
	if(obj > 255){
		DisplayContext();
		error("debug me");
	}
	return ToInteger(obj);
}

func hex(val){
	 var hexText = HexEncode(ToString(val));
	 if(len(hexText)%2){
		 return "0x0"+hexText;
	 }
	return "0x"+hexText;
}

func hexstr(str){
	return HexEncode(ToString(str));
}

func strstr(str1,str2){
	if(!str1){
		return nil;
	}
	var pos = IndexString(str1,str2);
	if(pos == -1){
		return nil;
	}
	return str1[pos:];
}

func substr(str,start,end=-1){
	if(!str){
		return nil;
	}
	if(end == -1 || end >= len(str)){
		end = len(str)-1;
	}
	if(start > end || start < 0){
		return "";
	}
	return str[start:end+1];
}

func insstr(str1,str2,i1,i2=-1){
	if(len(str1) == 0 || len(str2) == 0 || len(str1) < i1){
		error("check you parameter");
		return nil;
	}
	if(i2 < 0 || i2 > len(str1)){
		i2 = len(str1) - 1;
	}
	if(i1 > i2){
		error("check you parameter");
		return nil;
	}
	var result = str1[:i1];
	result += str2;
	result += str1[i2:];
	return result;
}

func tolower(str){
	if(!str){
		return nil;
	}
	return ToLowerString(str);
}

func toupper(str){
	if(!str){
		return nil;
	}
	return ToUpperString(str);
}

func crap(length,data=nil){
	if(!data){
		data = " ";
	}
	return RepeatString(data,length);
}

func strlen(str){
	if(str==nil){
		return 0;
	}
	return len(str);
}

func egrep(pattern, string,icase=false){
	if(!string){
		return nil;
	}
	var text_group = SplitString(string,"\n");
	var result = "";
	for v in text_group {
		if(len(v)> 0 && IsMatchRegexp(v,pattern,icase)){
			result +=v;
			result +="\n";
		}
	}
	if (len(result) == 0){
		return nil;
	}
	return ToString(result);
}

func ereg(pattern, string,multiline=false,icase=false){
	if(!string){
		return nil;
	}
	var text = string;
	if(!multiline){
		var lines = SplitString(string,"\n");
		text = lines[0];
	}
	return IsMatchRegexp(text,pattern,icase);
}

func ereg_replace(pattern, string,replace,icase=false){
	if(!string){
		return nil;
	}
	if(-1== IndexString(replace,"\\")){
        return RegExpReplace(string,pattern,replace,icase);
    }
    var list = SearchRegExp(string,pattern,icase);
    var reps = "";
    if(!list || len(list) == 0){
        return string;
    }
    var newr = "";
    for(var i = 0; i< len(replace);i++){
        if(replace[i]=='\\' && (i+1) < len(replace)){
            if(replace[i+1]>='0' && replace[i+1]<='9' ){
                if(replace[i+1]-'0' < len(list)){
                    newr += list[replace[i+1]-'0'];
                    i++;
                    continue;
                }
            }
			Println("ereg_replace :this may be have some error");
        }
        newr += replace[i];
    }
    return ReplaceString(string,list[0],newr,-1);
}

func eregmatch(pattern, string,icase=false){
	if(!string){
		return nil;
	}
	var result = SearchRegExp(string,pattern,icase);
	if(result != nil && len(result)==0){
		return nil;
	}
	return result;
}

func split(buffer, sep="\n",keep = -1){
	if(keep==-1){
		keep = true;
	}
	if(!buffer){
		return nil;
	}
	var list = SplitString(buffer,sep);
	for(var i = 0; i < len(list);i++){
		if(keep){
			list[i]+=sep;
		}else{
			if(sep=="\n"){
				list[i] = TrimRightString(list[i]);
			}
		}
	}
	return list;
}

func chomp(str){
	if(!str){
		return nil;
	}
	return TrimRightString(str," \t\n\r");
}

func int(other){
	return ToInteger(other);
}

func stridx(str,sub,pos = 0){
	if(pos > 0){
		str = str[pos:];
		var idx = IndexString(str,sub);
		if(idx>=0){
			idx += pos;
		}
		return idx;
	}
	return IndexString(str,sub);
}

func str_replace(string, find, replace,count = -1){
	return ReplaceString(string,find,replace,count);
}

func keys(obj){
	var list = {};
	if(typeof(obj)=="array"){
		#DisplayContext();
		Println("please check this may be have some bug**************");
		Println(ShortStack());
	}
	var res = NASLArray();
	for k,v in obj{
		list[k] = 1;
	}
	for k,v in list{
		res.add_var_to_list(k);
	}
	return res;
}

func max_index(obj){
	if(typeof(obj)=="NASLArray"){
		return len(obj.vector);
	}
	return 0;
}

func sort(obj){
	if(typeof(obj)!="array"){
		return obj;
	}
	var dic = {};
	for v in obj{
		dic[v] = 1;
	}
	var result = [];
	for k,v in dic{
		result = append(result,k);
	}
	return result;
}

func dump_ctxt(){
	DisplayContext();
}

func TARGET_IS_IPV6(){
	return ContainsBytes(get_host_ip(),":");
}

func islocalhost(){
	var env = HostEnv();
	return get_host_ip()=="127.0.0.1" || env["local_ip"] == get_host_ip();
}

func islocalnet(){
	var ip = get_host_ip();
	if(ContainsBytes(get_host_ip(),":")){
		error("not implement");
	}
	var list = SplitString(ip,".");
	var one = ToInteger(list[0]) & 0xFF;
	var tow = ToInteger(list[0]) & 0xFF;
	if(one == 172 && ((tow &0xfe)==16)){
		return true;
	}
	if(one == 192 && tow == 168){
		return true;
	}
	return false;
}

#crypto

func prf_sha256(secret,seed,label,outlen){
	return TLSRPF(secret,label+seed,"sha256",outlen);
}

func prf_sha384(secret,seed,label,outlen){
	return TLSRPF(secret,label+seed,"sha384",outlen);
}

func tls1_prf(secret,seed,label,outlen){
	return TLS1PRF(secret,label+seed,outlen);
}

func HMAC_MD5(data,key){
	return HMACMethod("md5",key,data);
}

func HMAC_SHA1(data,key){
	return HMACMethod("sha1",key,data);
}

func HMAC_SHA256(data,key){
	return HMACMethod("sha256",key,data);
}

func HMAC_SHA384(data,key){
	return HMACMethod("sha384",key,data);
}

func HMAC_SHA512(data,key){
	return HMACMethod("sha512",key,data);
}

func rc4_encrypt(key,data){
	var cipher = CipherOpen("rc4",1,key,"",true);
	return CipherUpdate(cipher,data);
}

func aes128_cbc_encrypt(data,key,iv){
	var cipher = CipherOpen("aes-128-cbc",1,key,iv,true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func aes256_cbc_encrypt(data,key,iv){
	var cipher = CipherOpen("aes-256-cbc",1,key,iv,true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func aes128_ctr_encrypt(data,key,iv){
	var cipher = CipherOpen("aes-128-ctr",1,key,iv,true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func aes256_ctr_encrypt(data,key,iv){
	var cipher = CipherOpen("aes-256-ctr",1,key,iv,true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func aes128_gcm_encrypt(data,key,iv){
	var cipher = CipherOpen("aes-128-gcm",1,key,iv,true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func aes256_gcm_encrypt(data,key,iv){
	var cipher = CipherOpen("aes-256-gcm",1,key,iv,true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func DES(data,key){
	var cipher = CipherOpen("des",1,key,"",true);
	return CipherUpdate(cipher,data)+CipherFinal(cipher);
}

func get_port_state(port){
	var key = ","+port;
	var result = HostEnv();
	return ContainsBytes(result["opened_tcp"],key);
}

func get_tcp_port_state(port){
	var key = ","+port;
	var result = HostEnv();
	return ContainsBytes(result["opened_tcp"],key);
}

func get_udp_port_state(port){
	var key = ","+port;
	var result = HostEnv();
	return ContainsBytes(result["opened_udp"],key);
}

func this_host(){
	var result = HostEnv();
	return result["local_ip"];
}

func get_opened_tcp(){
	var result = HostEnv();
	var ret = [];
	var list = SplitString(result["opened_tcp"],",");
	for v in list{
		if(len(v) > 0){
			ret = append(ret,ToInteger(v));
		}
	}
	return ret;
}

func this_host_name(){
	return GetHostName();
}

#add_host_name(hostname: tmp, source: "SSL/TLS server certificate")
func add_host_name(hostname,source){
	var map = get_kb_list("HostNameManger");
	if(map==nil){
		map = {};
	}
	map[hostname] = source;
	return;
}

func get_host_name(){
	return get_host_ip();
}

func get_host_names(){
	var map = get_kb_list("HostNameManger");
	if(map==nil){
		return [];
	}
	return keys(map);
}

func get_host_name_source(hostname){
	var map = get_kb_list("HostNameManger");
	if(map==nil){
		return "";
	}
	var source = map[hostname];
	if(source==nil){
		return "";
	}
	return source;
}

func resolve_host_name(hostname){
	return ResolveHostName(hostname);
}

func resolve_hostname_to_multiple_ips(hostname){
	return ResolveHostNameToList(hostname);
}

func get_port_transport(port){
	if(ToInteger(port)==443){
		return ENCAPS_TLSv12;
	}
	if(ToInteger(port)==80){
		return 1;
	}
	var kb = get_kb_item("Transports/TCP/"+port);
	if(kb == nil){
		var soc = TCPConnect(get_host_ip(),ToInteger(port),15,true);
		if(soc != nil){
			replace_kb_item("Transports/TCP/"+port,ENCAPS_TLSv12);
			return ENCAPS_TLSv12;
		}else{
			replace_kb_item("Transports/TCP/"+port,ENCAPS_IP);
			return ENCAPS_IP;
		}
	}
	return kb;
}

func http_open_socket(port){
	var ip = get_host_ip();
	var isTLS = get_port_transport(port) > 1;
	var soc = TCPConnect(ip,ToInteger(port),30,isTLS);
	if(soc){
		ConnSetReadTimeout(soc,5);
		ConnSetWriteTimeout(soc,5);
	}else{
		Println("http_open_socket failed :",ip+":"+port);
	}
	return soc;
}

func http_close_socket(soc){
	close(soc);
}

func display(msg...){
	var msgs = "";
	for v in msg{
		msgs += ToString(v);
	}
	Println(msgs);
}

func cert_open(cert){
	return X509Open(cert);
}

func cert_close(cert){
	return close(cert);
}

func cert_query(cert,cmd,idx=0){
	return X509Query(cert,cmd,idx);
}

func plugin_run_find_service(){

}

func plugin_run_synscan(){

}

func plugin_run_openvas_tcp_scanner(){
	
}

#wmi support

func _split_keys_from_query_string(query){
    query = TrimString(query,"\t ");
    var index = IndexString(query," ");
    if(index==-1){
        return "";
    }
    query = query[index:];
    index = IndexString(query," from");
    if (index == -1){
        index = IndexString(query," FROM");
    }
    if(index==-1){
        return "";
    }
    query = query[:index];
    query = TrimString(query," \t");
    if(ContainsBytes(query,"*")){
        return "";
    }
    var list = SplitString(query,",");
    var keys =[];
    for v in list{
        keys = append(keys,TrimString(v," \t"));
    }
    return keys;
}

# "root\rsop\computer"
# Get-WmiObject -Query {SELECT Name from Win32_Processor}| Format-List -Property *
func WMIQuery(handle,query,namespace=nil){
    var cmd = "Get-WmiObject -Query {"+query+"}"+"| Format-List -Property *";
    if(namespace){
        cmd = "Get-WmiObject -Query {"+query+"}"+" -Namespace "+namespace+" | Format-List -Property *";
    }
    var result = WinRMCommand(handle,cmd,"",true);
    var ret = [];
    if (result == nil){
        return nil;
    }
    if (result.ExitCode != 0 || len(result.StdErr) > 0){
        Println(result);
        return nil;
    }
    var out = TrimString(result.Stdout,"\r\n\t ");
    var lines = SplitString(out,"\n");
    var item = {};
    for li,line in lines{
        line = TrimString(line,"\r\n\t ");
        var pos = IndexString(line,":");
        if(pos ==-1){
            continue;
        }
        if(li+1 < len(lines)){
            if(HasPrefixString(lines[li+1],"       ")){
                line += TrimString(lines[li+1],"\r\n\t ");
            }
        }
        var key =   line[:pos];
        var value = line[pos+1:];
        key = TrimString(key,"\r\n\t ");
        value = TrimString(value,"\r\n\t ");
        if(key=="PSComputerName"){
            if(len(item)){
                ret = append(ret,item);
                item = {};
            }
            continue;
        }
        if (HasPrefixString(key,"__")){
            continue;
        }
        item[key] = value;
    }
    if(len(item)){
        ret = append(ret,item);
    }
    return ret;
}

func _print_wmi_result(keys,item){
    var result = "";
    for k,v in keys{
        if(k>0){
            result += "|";
        }
        result += item[v];
    } 
    result += "\n";
    return result;
}

func wmi_versioninfo(){
    return "1.0";
}

func wmi_connect(username,password,ns=nil,host=""){
    if(host == "" || host == nil){
        host = get_host_ip();
    }
    var winrm = CreateWinRM(host,5985,username,password,false,true,"","","",15,true);
    if(winrm == nil){
        return nil;
    }
    return winrm;
}

func wmi_close(handle){
    close(handle);
}

func wmi_connect_reg(host="",username,password){
    if(host == "" || host == nil){
        host = get_host_ip();
    }
    var winrm = CreateWinRM(host,5985,username,password,false,true,"","","",15,true);
    if(winrm == nil){
        return nil;
    }
    return winrm;
}

func wmi_query(handle,query,namespace=nil){
    var result = WMIQuery(handle,query,namespace);
    if(result == nil || len(result)==0){
        return "";
    }
    var keys = _split_keys_from_query_string(query);
    if(len(keys) ==0){
        keys = [];
        for k,v in result[0]{
            keys = append(keys,k);
        }
    }
    var pts = "";
    for v in keys{
        if(len(pts)){
            pts += "|";
        }
        pts += v;
    }
    pts += "\n";
    for v in result{
        pts += _print_wmi_result(keys,v);
    }
    return pts;
}

func wmi_query_rsop(handle,query){
    return wmi_query(handle,query,"root\rsop\computer");
}

func ExecuteRegCommand(handle,cmd){
    var result = WinRMCommand(handle,cmd,"",false);
    if (result ==nil){
        return nil;
    }
    if(result.ExitCode != 0 || len(result.StdErr)>0){
        Println(result,cmd);
        return nil;
    }
    return result.Stdout;
}

func _warp_qute(str){
    str = ReplaceAllString(str,"\"","\\\"");
    return "\""+str+"\"";
}

func RegParseValue(line){
    if(!HasPrefixString(line,"    ")){
        return nil;
    }
    line = line[4:];
    var pos = IndexString(line,"    REG_");
    if(pos == -1){
        return nil;
    }
    var name = line[:pos];
    line =line[pos+4:];
    pos = IndexString(line," ");
    if(pos == -1){
        return nil;
    }
    var type = line[:pos];
    var value = "";
    if(pos+4 < len(line)){
        line = line[pos+4:];
        value = line;
    }
    return {"name":name,"type":type,"value":value};
}

func RegParseKey(key){
    var pos = IndexString(key,"\\");
    if(pos == -1){
        return key;
    }
    var sub = key[pos:];
    if(HasPrefixString(key,"HKEY_LOCAL_MACHINE\\")){
        return "HKLM"+sub;
    }
    if(HasPrefixString(key,"HKEY_CURRENT_USER\\")){
        return "HKCU"+sub;
    }
    if(HasPrefixString(key,"HKEY_CLASSES_ROOT\\")){
        return "HKCR"+sub;
    }
    if(HasPrefixString(key,"HKEY_CURRENT_CONFIG\\")){
        return "HKCC"+sub;
    }
    if(HasPrefixString(key,"HKEY_USERS\\")){
        return "HKU"+sub;
    }
    return key;
}

func RegEnumKey(handle,key){
    var cmd = "REG QUERY "+_warp_qute(key);
    var buf = ExecuteRegCommand(handle,cmd);
    if(!buf || len(buf)==0){
        return nil;
    }
    var result = [];
    var list = SplitString(buf,"\r\n");
    for v in list{
        if(len(v)==0 || v[0]==' ' || v[0]=='\t'){
            continue;
        }
        result = append(result,RegParseKey(v));
    }
    return result;
}


func RegEnumValue(handle,key){
    var cmd = "REG QUERY "+_warp_qute(key);
    var buf = ExecuteRegCommand(handle,cmd);
    if(!buf || len(buf)==0){
        return nil;
    }
    var result = [];
    var list = SplitString(buf,"\r\n");
    var parse = false;
    for v in list{
        if(len(v) == 0){
            continue;
        }
        if (v[0]==' ' || v[0]=='\t'){
            if(!parse){
                continue;
            }
            var val = RegParseValue(v);
            result = append(result,val);
        }else{
            if(parse){
                break;
            }
            if(RegParseKey(v)==key){
                parse = true;
                continue;
            }
        }
    }
    return result;
}

func RegGetValue(handle,key,name){
    var cmd = "REG QUERY "+_warp_qute(key)+" -v "+_warp_qute(name);
    var buf = ExecuteRegCommand(handle,cmd);
    if(!buf || len(buf)==0){
        return nil;
    }
    var list = SplitString(buf,"\r\n");
    var parse = false;
    for v in list{
        if(len(v) == 0){
            continue;
        }
        if (v[0]==' ' || v[0]=='\t'){
            if(!parse){
                continue;
            }
            return RegParseValue(v);
        }else{
            if(parse){
                break;
            }
            if(RegParseKey(v)==key){
                parse = true;
                continue;
            }
        }
    }
    return nil;
}

func RegAddKey(handle,key){
    var cmd ="REG ADD "+ _warp_qute(key);
    var buf = ExecuteRegCommand(handle,cmd);
    if(!buf){
        return false;
    }
    return true;
}

func RegAddValue(handle,key,value){
    var cmd ="REG ADD "+_warp_qute(key);
    cmd += " /v ";
    cmd += _warp_qute(value["name"]);
    cmd += " /t ";
    cmd += _warp_qute(value["type"]);
    cmd += " /d ";
    cmd += _warp_qute(value["value"]);
    var buf = ExecuteRegCommand(handle,cmd);
    if(!buf){
        return false;
    }
    return true;
}

func WinRootHiveToPath(hive){
    var hive_roots = {0x80000002:"HKLM",0x80000000:"HKCR",0x80000001:"HKCU",0x80000003:"HKU",0x80000005:"HKCC"};
    if(hive==0 || hive == nil){
        return "HKLM";
    }
    return hive_roots[ToInteger(hive)];
}

func wmi_reg_enum_key(wmi_handle,key,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegEnumKey(wmi_handle,fullkey);
    if (res == nil){
        return nil;
    }
    var txt = "";
    for v in res{
        if(len(txt)){
            txt += "|";
        }
        txt += v;
    }
    return txt;
}

func wmi_reg_enum_value(wmi_handle,key,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegEnumValue(wmi_handle,fullkey);
    if (res == nil){
        return nil;
    }
    var txt = "";
    for v in res{
        if(len(txt)){
            txt += "|";
        }
        txt += v["name"];
    }
    return txt;
}

func wmi_reg_get_sz(wmi_handle,key,key_name,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegGetValue(wmi_handle,fullkey,key_name);
    if (res == nil){
        return nil;
    }
    return res["value"];
}

func _binay_convert(str){
    var res = 0;
    for(var i =0;i<len(str);i++){
        if(str[i]=='1'){
            res |=(1<<i);
        }
    }
    return res;
}

func _one_byte(iv){
    if(iv==0){
        return "0";
    }
    return byte(iv);
}

func _reg_binary_to_string(str){
    var result = "",temp = "";
    for{
        if(len(str)<=8){
            result += _one_byte(_binay_convert(str));
            return result;
        }
        temp = str[:8];
        str =str[8:];
        result += _one_byte(_binay_convert(temp));
    }
}

func wmi_reg_get_bin_val(wmi_handle,key,val_name,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegGetValue(wmi_handle,fullkey,val_name);
    if (res == nil){
        return nil;
    }
    return _reg_binary_to_string(res["value"]);
}

func wmi_reg_get_dword_val(wmi_handle,key,val_name,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegGetValue(wmi_handle,fullkey,val_name);
    if (res == nil){
        return nil;
    }
    return ToInteger(res["value"]);
}

func wmi_reg_get_ex_string_val(wmi_handle,key,val_name,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegGetValue(wmi_handle,fullkey,val_name);
    if (res == nil){
        return nil;
    }
    return res["value"];
}

func wmi_reg_get_mul_string_val(wmi_handle,key,val_name,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    var res = RegGetValue(wmi_handle,fullkey,val_name);
    if (res == nil){
        return nil;
    }
    return ReplaceAllString(res["value"],"\\0","|");
}

func wmi_reg_set_ex_string_val(wmi_handle,key,val_name,val="",hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    return RegAddValue(wmi_handle,fullkey,{"name":val_name,"type":"REG_EXPAND_SZ","value":ToString(val)});
}

func wmi_reg_set_string_val(wmi_handle,key,val_name,val="",hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    return RegAddValue(wmi_handle,fullkey,{"name":val_name,"type":"REG_SZ","value":val});
}

func wmi_reg_create_key(wmi_handle,key,hive=0){
    var hive = WinRootHiveToPath(hive);
    var fullkey = hive;
    if(key[0] != '\\'){
        fullkey += "\\";
    }
    fullkey +=key;
    return RegAddKey(wmi_handle,fullkey);
}

func win_cmd_exec(host="",username,password,cmd){
    if(host == "" || host == nil){
        host = get_host_ip();
    }
    var winrm = CreateWinRM(host,5985,username,password,false,true,"","","",15,true);
    if(winrm == nil){
        return nil;
    }
    var res = WinRMCommand(winrm,cmd,"",false);
    if (res == nil){
        return nil;
    }
    if (res.ExitCode != 0){
        return res.StdErr;
    }
    return res.Stdout;
}

func AppendUInt32ToBuffer(buf,val,bigEndian){
    if(bigEndian){
        return append(buf,(val>>24)&0xFF,(val>>16)&0xFF,(val>>8)&0xFF,val&0xFF);
    }
    return append(buf,val&0xFF,(val>>8)&0xFF,(val>>16)&0xFF,(val>>24)&0xFF);
}

func AppendUInt16ToBuffer(buf,val,bigEndian){
    if(bigEndian){
        return append(buf,(val>>8)&0xFF,val&0xFF);
    }
    return append(buf,val&0xFF,(val>>8)&0xFF);
}


func ReadUInt32(buf,offset,bigEndian=true){
    var val ;
    if(bigEndian){
        val = ((buf[0+offset]&0xFF)<<24)|((buf[1+offset]&0xFF)<<16) |((buf[2+offset]&0xFF)<<8) |buf[3+offset]&0xFF;
    }else{
        val = ((buf[3+offset]&0xFF)<<24)|((buf[2+offset]&0xFF)<<16) |((buf[1+offset]&0xFF)<<8) |buf[0+offset]&0xFF;
    }
    return val &0xFFFFFFFF;
}


func ReadUInt16(buf,offset,bigEndian=true){
    var val ;
    if(bigEndian){
        val = ((buf[0+offset]&0xFF)<<8) |buf[1+offset]&0xFF;
    }else{
        val = ((buf[1+offset]&0xFF)<<8) |buf[offset]&0xFF;
    }
    return val&0xFFFF;
}

func WriteUInt32(buf,offset,val,bigEndian=true){
    if(bigEndian){
        buf[offset]   = byte((val>>24)&0xFF);
        buf[offset+1] = byte((val>>16)&0xFF);
        buf[offset+2] = byte((val>>8)&0xFF);
        buf[offset+3] = byte(val&0xFF);
        return buf;
    }
    buf[offset+3]   = byte((val>>24)&0xFF);
    buf[offset+2] = byte((val>>16)&0xFF);
    buf[offset+1] = byte((val>>8)&0xFF);
    buf[offset] = byte(val&0xFF);
    return buf;
}

func WriteUInt16(buf,offset,val,bigEndian=true){
    if(bigEndian){
        buf[offset] = byte((val>>8)&0xFF);
        buf[offset+1] = byte(val&0xFF);
        return buf;
    }
    buf[offset+1] = byte((val>>8)&0xFF);
    buf[offset] = byte(val&0xFF);
    return buf;
}

var IPV6_VERSION_MASK  = 0xf0;
var IPV6_FLOWINFO_MASK = 0xffffff0f;
var IPV6_FLOWLABEL_MASK= 0xffff0f00;
var IPV6_FLOW_ECN_MASK = 0x00003000;
var IP6FLOW_DSCP_MASK  = 0x0fc00000;
var IP6FLOW_DSCP_SHIFT = 22;

func allocte_ipv6_header(){
    var hdr= MakeBytes(40);
    hdr[0] = byte(0x60);
    return hdr;
}

func set_ipv6_dscp(hdr,dsc){
    dsc = (dsc & 0x3F)<<22;
    hdr=WriteUInt32(hdr,0,ReadUInt32(hdr,0)|(dsc&0xFFFFFFFF));
    return hdr;
}

func get_ipv6_dscp(hdr){
    var val = ReadUInt32(hdr,0);
    return (val >>22)&0x3F;
}

func set_ipv6_enc(hdr,enc){
    enc = (enc & 0x3F)<<20;
    hdr=WriteUInt32(hdr,0,ReadUInt32(hdr,0)|(enc&0xFFFFFFFF));
    return hdr;
}

func get_ipv6_enc(hdr){
    var val = ReadUInt32(hdr,0);
    return (val >>20)&0x3F;
}

func set_ipv6_flow(hdr,flow){
    flow = (flow & 0xFFFFF);
    hdr=WriteUInt32(hdr,0,ReadUInt32(hdr,0)|(flow&0xFFFFFFFF));
    return hdr;
}

func get_ipv6_flow(hdr){
    var val = ReadUInt32(hdr,0);
    return val&0xFFFFF;
}

func set_ipv6_plen(hdr,length){
    hdr=WriteUInt16(hdr,4,length&0xFFFF);
    return hdr;
}

func get_ipv6_plen(hdr){
    return ReadUInt16(hdr,4);
}


func set_ipv6_nxt(hdr,nxt){
    hdr[6] = byte(nxt);
    return hdr;
}

func set_ipv6_hopslimit(hdr,hlmit){
    hdr[7] = byte(hlmit);
    return hdr;
}

func set_ipv6_src(hdr,src){
    for(var i = 0; i < 16;i++){
        hdr[8+i] = src[i];
    }
    return hdr;
}
func set_ipv6_dst(hdr,dst){
    for(var i = 0; i < 16;i++){
        hdr[24+i] = dst[i];
    }
    return hdr;
}

func allocate_ipv4_header(size){
    var hdr = MakeBytes(size);
    hdr[0] = 0x40+size/4;
    return hdr;
}

func set_ipv4_hl(hdr,hl){
    hdr[0] = 0x40 +(hl & 0xF);
    return hdr;
}

func get_ipv4_hl(hdr){
    return ToInteger((hdr[0]&0xF));
}

func set_ipv4_tos(hdr,tos){
    hdr[1] = tos&0xFF;
    return hdr;
}
func get_ipv4_tos(hdr){
    return hdr[1]&0xFF;
}

func set_ipv4_length(hdr,length){
    length = (length &0xFFFF);
    hdr=WriteUInt16(hdr,2,length);
    return hdr;
}

func get_ipv4_length(hdr){
    return ReadUInt16(hdr,2);
}

func set_ipv4_id(hdr,id){
    id = (id &0xFFFF);
    hdr=WriteUInt16(hdr,4,id);
    return hdr;
}

func set_ipv4_off_flags(hdr,flags){
    flags = (flags &7) << 13;
    hdr=WriteUInt16(hdr,6,ReadUInt16(hdr,6)|flags);
    return hdr;
}

func set_ipv4_off(hdr,off){
    off = (off &0x1fff);
    hdr=WriteUInt16(hdr,6,ReadUInt16(hdr,6)|off);
    return hdr;
}

func set_ipv4_ttl(hdr,ttl){
    ttl = (ttl&0xFF);
    hdr[8] = byte(ttl);
    return hdr;
}

func get_ipv4_ttl(hdr){
    return  ToInteger(hdr[8]);
}

func set_ipv4_protocol(hdr,proto){
    proto = (proto&0xFF);
    hdr[9] = byte(proto);
    return hdr;
}

func get_ipv4_protocol(hdr){
    return ToInteger(hdr[9]);
}

func set_ipv4_src(hdr,src){
    hdr=WriteUInt32(hdr,12,src);
    return hdr;
}

func set_ipv4_dst(hdr,dst){
    hdr=WriteUInt32(hdr,16,dst);
    return hdr;
}

func ipv4_add_option(hdr,type,length,value){
    if(len(value)+2 != length){
        error("invalid paramter for ipv4_add_option");
    }
    opt = MakeBytes(length);
    opt[0] =  byte(type);
    opt[1] =  byte(length);
    for i,v in value {
        opt[2+i] = v;
    }
    hdr = append(hdr,opt);
    set_ipv4_hl(hdr,len(hdr)/4);
    update_ipv4_checksum(hdr);
    return hdr;
}

func get_ipv4_checksum(hdr){
    return ReadUInt16(hdr,10);
}

func cacl_ip_checksum(hdr,size){
    var sum = 0,s = 0;
    for (var i =0;(i+1)<size;i+=2){
        s = ReadUInt16(hdr,i,false);
        sum += s;
    }
    if(size % 2){
        sum += ToInteger(hdr[size-1]);
    }
    sum = (sum>>16) + (sum &0xFFFF);
    sum += (sum >>16);
    return (~(sum))&0xFFFF;
}

func update_ipv4_checksum(hdr){
    hdr = WriteUInt16(hdr,10,0);
    var hl = get_ipv4_hl(hdr);
    hdr=WriteUInt16(hdr,10,cacl_ip_checksum(hdr,hl*4),false);
    return hdr;
}


func build_ipv6_header(ip6_tc=0,ip6_fl=0,ip6_p=0,ip6_hlim=64,ip6_src,ip6_dst,ip_plen){
    var hdr = allocte_ipv6_header();
    if(ip6_fl){
        set_ipv6_flow(hdr,ip6_fl);
    }
    if(ip6_tc){
        set_ipv6_dscp(hdr,ip6_tc&0xFC);
        set_ipv6_enc(hdr,ip6_tc&0x3);
    }
    if(ip6_hlim){
        hdr = set_ipv6_hopslimit(hdr,ip6_hlim);
    }
    set_ipv6_nxt(hdr,ip6_p);
    set_ipv6_plen(hdr,ip_plen);
    set_ipv6_src(hdr,ip6_src);
    set_ipv6_dst(hdr,ip6_dst);
    return hdr;
}

#fe80::872:545c:cf3b:e0f5
#ff02::16
#1050:0:0:0:5:600:300c:326b
func ipv6_address_chunk_parser(chunk){
    var list = SplitString(chunk,":");
    var hex = "0000";
    var res = "";
    for v in list{
        hex = "0000"+v;
        hex = hex[len(hex)-4:];
        res += hex;
    }
    return res;
}
func ipv6_string_to_address(src){
    if(ContainsString(src,".")){
        error("please process ipv4-mapped ipv6 addresss..."+src);
    }
    var result = "";
    var pos = IndexString(src,"::");
    if(pos != -1){
        if(pos+2 == len(src)){
            result = ipv6_address_chunk_parser(src[:pos]);
            result += RepeatString("0",32-len(result));
        }else{
            result = ipv6_address_chunk_parser(src[:pos]);
            var part = ipv6_address_chunk_parser(src[pos+2:]);
            result += RepeatString("0",32-len(result)-len(part));
            result += part;

        }
    }else{
        result = ipv6_address_chunk_parser(src);
    }
    return HexDecodeString(result);
}

func ipv6_address_string(src){
    if(len(src)<16){
        return "";
    }
    var hex = HexEncode(src);
    var result = "";
    for (var i = 0; i < 32;i+=4){
        if(hex[i:i+4] == "0000"){
            if(HasSuffixBytes(result,"::")){
                continue;
            }
            result += ":";
            continue;
        }
        result += TrimLeftBytes(hex[i:i+4],"0");
        result += ":";
    }
    return result[:len(result)-1];
}

func ipv4_string_to_address(src){
    var list = SplitString(src,".");
    var result = 0;
    for i,v in list{
        result += ToInteger(v)<<((3-i)*8);
    }
    return result &0xFFFFFFFF;
}

func ipv4_address_to_string(src){
    if (typeof(src)=="integer"){
        src = AppendUInt32ToBuffer(bytes(),src);
    }
    return ToString(ToInteger(src[0]))+"."+ToString(ToInteger(src[1]))+"."+ToString(ToInteger(src[2]))+"."+ToString(ToInteger(src[3]));
}

func build_ipv4_header(ip_hl,ip_tos,ip_len,ip_id,ip_off_flags,ip_off,ip_ttl,ip_p,ip_src,ip_dst){
    var hdr = allocate_ipv4_header(ip_hl*4);
    if(ip_hl){
        set_ipv4_hl(hdr,ip_hl);
    }
    if(ip_id){
        set_ipv4_id(hdr,ip_id);
    }
    if(ip_len){
        set_ipv4_length(hdr,ip_len);
    }
    if(ip_off_flags){
        set_ipv4_off_flags(hdr,ip_off_flags);
    }
    if(ip_off){
        set_ipv4_off(hdr,ip_off);
    }
    set_ipv4_protocol(hdr,ip_p);
    set_ipv4_src(hdr,ip_src);
    set_ipv4_dst(hdr,ip_dst);
    if(ip_tos){
        set_ipv4_tos(hdr,ip_tos);
    }
    if(ip_ttl){
        set_ipv4_ttl(hdr,ip_ttl);
    }
    update_ipv4_checksum(hdr);
    return hdr;
}

func build_udp(ip,src_port,dst_port,data){
    if(ip[0]==0x60){
        var ps_head = MakeBytes(36);
        ps_head[33] = 17;
        ps_head = WriteUInt16(ps_head,34,8+len(data));
        CopyBytes(ps_head,ip[8:24]);
        CopyBytes(ps_head[16:],ip[24:40]);
        var udp = MakeBytes(8+len(data));
        udp = WriteUInt16(udp,0,src_port);
        udp = WriteUInt16(udp,2,dst_port);
        udp = WriteUInt16(udp,4,len(udp));
        CopyBytes(udp[8:],data);
        var sum = cacl_ip_checksum(append(ps_head,udp),36+len(udp));
        udp = WriteUInt16(udp,6,sum,false);
        return udp;
    }else{
        var ps_head = MakeBytes(12);
        ps_head[9] = 17;
        ps_head = WriteUInt16(ps_head,10,8+len(data));
        CopyBytes(ps_head,ip[12:16]);
        CopyBytes(ps_head[4:8],ip[16:20]);
        var udp = MakeBytes(8+len(data));
        udp = WriteUInt16(udp,0,src_port);
        udp = WriteUInt16(udp,2,dst_port);
        udp = WriteUInt16(udp,4,len(udp));
        CopyBytes(udp[8:],data);
        var sum = cacl_ip_checksum(append(ps_head,udp),12+len(udp));
        udp = WriteUInt16(udp,6,sum,false);
        return udp;
    }   
}

func build_tcp_header(src_port,dst_port,seq,ack,flags,win,urp){
    var tcp = MakeBytes(20);
    tcp = WriteUInt16(tcp,0,src_port);
    tcp = WriteUInt16(tcp,2,dst_port);
    tcp = WriteUInt32(tcp,4,seq);
    tcp = WriteUInt32(tcp,8,ack);
    tcp[12] = (5<<4 &0xF0);
    tcp[13] = (flags &0x3F);
    tcp = WriteUInt16(tcp,14,win);
    tcp = WriteUInt16(tcp,18,urp);
    return tcp;
}

func add_tcp_option(tcp,type,length,value){
    var opt ;
    if(type == 1){
        opt = MakeBytes(1);
        opt[0] = 0x1;
    }else{
        opt = MakeBytes(length);
        opt[0] = type;
        opt[1] = length;
        CopyBytes(opt[2:],value);
    }
    tcp = append(tcp,opt);
    tcp[12] = ((len(tcp)/4)<<4)& 0xF0;
    return tcp;
}

func update_tcp_checksum(ip,tcp,data){
    if(ip[0]==0x60){
        var ps_head = MakeBytes(36);
        ps_head[33] = 6;
        var size = len(tcp);
        if(data != nil){
            size += len(data);
        }
        ps_head = WriteUInt16(ps_head,34,size);
        CopyBytes(ps_head,ip[8:24]);
        CopyBytes(ps_head[16:],ip[24:40]);
        tcp = WriteUInt16(tcp,16,0);
        var total = append(ps_head,tcp);
        if(data!= nil){
            total = append(total,data);
        }
        var sum = cacl_ip_checksum(total,36+size);
        tcp = WriteUInt16(tcp,16,sum,false);
        return tcp;    
    }else{
        var ps_head = MakeBytes(12);
        ps_head[9] = 6;
        var size = len(tcp);
        if(data != nil){
            size += len(data);
        }
        ps_head = WriteUInt16(ps_head,10,size);
        CopyBytes(ps_head,ip[12:16]);
        CopyBytes(ps_head[4:8],ip[16:20]);
        tcp = WriteUInt16(tcp,16,0);
        var total = append(ps_head,tcp);
        if(data!= nil){
            total = append(total,data);
        }
        var sum = cacl_ip_checksum(total,12+size);
        tcp = WriteUInt16(tcp,16,sum,false);
        return tcp;    
    }
}


#ip6_tc=0,ip6_fl=0,ip6_p=0,ip6_hlim=64,ip6_src,ip6_dst,ip_plen
func forge_ipv6_packet(data="",ip6_v=6,ip6_tc,ip6_fl,ip6_p,ip6_hlim,ip6_src,ip6_dst){
    return build_ipv6_header(ip6_tc,ip6_fl,ip6_p,ip6_hlim,
                                ipv6_string_to_address(ip6_src),
                                ipv6_string_to_address(ip6_dst),len(data));
}

func dump_ipv6_packet(packet){
    Println(HexDumpBytes(packet));
}

#func build_ipv4_header(ip_hl,ip_tos,ip_len,ip_id,ip_off_flags,ip_off,ip_ttl,ip_p,ip_src,ip_dst)
func forge_ip_packet(data="",ip_hl=5,ip_v=4,ip_tos=0,ip_id=0,ip_off=0,
ip_ttl=64,ip_p=0,ip_sum=0,ip_src="",ip_dst="",ip_len=0){
    var ip_len = len(data)+ip_hl*4;
    if(ip_id == 0){
        ip_id = rand();
    }
    hdr = build_ipv4_header(ip_hl,ip_tos,ip_len,ip_id,(ip_off>>13),
                        ip_off,ip_ttl,ip_p,ipv4_string_to_address(ip_src),ipv4_string_to_address(ip_dst));
    if(ip_sum != 0){
        hdr=WriteUInt16(hdr,10,ip_sum,true);
    }else{
        hdr=update_ipv4_checksum(hdr);
    }
    return hdr;
}

func forge_ip_v6_packet(data="",ip6_v=6,ip6_tc,ip6_fl,ip6_p,ip6_hlim,ip6_src,ip6_dst,ip_len=0){
    return build_ipv6_header(ip6_tc,ip6_fl,ip6_p,ip6_hlim,
                                ipv6_string_to_address(ip6_src),
                                ipv6_string_to_address(ip6_dst),len(data));
}

func get_ip_element(ip,element){
    if(!ip || !element){
        return nil;
    }
    switch(element){
        case "ip_v":{
            return 4;
        }
        case "ip_id":{
            return ReadUInt16(ip,4);
        }
        case "ip_hl":{
            return get_ipv4_hl(ip);
        }
        case "ip_tos":{
            return get_ipv4_tos(ip);
        }
        case "ip_len":{
            return ReadUInt16(ip,2);
        }
        case "ip_off":{
            return ReadUInt16(ip,6);
        }
        case "ip_ttl":{
            return get_ipv4_ttl(ip);
        }
        case "ip_p":{
            return get_ipv4_protocol(ip);
        }
        case "ip_sum":{
            return get_ipv4_checksum(ip);
        }
        case "ip_src":{
            return ipv4_address_to_string(ip[12:16]);
        }
        case "ip_dst":{
            return ipv4_address_to_string(ip[16:20]);
        }
    }
    return nil;
}

func set_ip_elements(ip,ip_hl=nil,ip_v=nil,ip_tos=nil,ip_id=nil,ip_off=nil,
                     ip_ttl=nil,ip_p=nil,ip_sum=nil,ip_src=nil,ip_dst=nil){
    if (!ip){
        return nil;
    }
    if(ip_hl){
        ip = set_ipv4_hl(ip,ip_hl);
    }
    if(ip_tos){
        ip = set_ipv4_tos(ip,ip_tos);
    }
    if(ip_id){
        ip = set_ipv4_id(ip,ip_id);
    }
    if(ip_off){
        ip = set_ipv4_off_flags(ip,ip_off>>13);
        ip = set_ipv4_off(ip,ip_off);
    }
    if(ip_ttl){
        ip = set_ipv4_ttl(ip,ip_ttl);
    }
    if(ip_p){
        ip = set_ipv4_protocol(ip,ip6_p);
    }
    if(ip_sum){
        ip = WriteUInt16(ip,10,ip_sum);
    }
    if(ip_src){
        ip = set_ipv4_src(ip,ipv4_string_to_address(ip_src));
    }
    if(ip_dst){
         ip = set_ipv4_dst(ip,ipv4_string_to_address(ip_dst));
    }
    return ip;
}

func display_ip_layer(ip){
    if(!ip || len(ip)<20){
        return nil;
    }
    if(ip[0]>>4 == 4){ #ipv4
        Println(" ");
        Println("Version:\t4");
        Println("Header Length:\t"+get_ipv4_hl(ip)*4);
        Println("TOS:\t\t"+get_ipv4_tos(ip));
        Println("Total Length:\t"+get_ipv4_length(ip));
        Println("ID:\t\t"+ReadUInt16(ip,4));
        Println("Flags:\t\t"+(ReadUInt16(ip,6)>>13&0x7));
        Println("Offset:\t\t"+(ReadUInt16(ip,6)&0x3F));
        Println("TTL:\t\t"+get_ipv4_ttl(ip));
        Println("Protocol:\t"+get_ipv4_protocol(ip));
        Println("Header ChckSum:\t"+HexEncode(ReadUInt16(ip,10)));
        Println("Src IP:\t\t"+ipv4_address_to_string(ip[12:16]));
        Println("Dst IP:\t\t"+ipv4_address_to_string(ip[16:20]));
        return ip[get_ipv4_hl(ip)*4:];
    }
    if(ip[0]>>4 == 6){
        if(len(ip)<40){
            return nil;
        }
        Println(" ");
        Println("Version:\t6");
        Println("DSCP:\t\t"+get_ipv6_dscp(ip));
        Println("ENC:\t\t"+get_ipv6_enc(ip));
        Println("Payload Length:\t"+get_ipv6_plen(ip));
        Println("Next Header:\t"+ToInteger(ip[6]));
        Println("Hop Limit:\t"+ToInteger(ip[7]));
        Println("Src IP:\t\t"+ipv6_address_string(ip[8:24]));
        Println("Dst IP:\t\t"+ipv6_address_string(ip[24:40]));
        return ip[40:];
    }
    return nil;
}

func dump_ip_packet(packet){
    display_ip_layer(packet);
}

func dump_ip_v6_packet(packet){
   display_ip_layer(packet);
}

#func build_tcp_header(src_port,dst_port,seq,ack,flags,win,urp)
func forge_tcp_packet(ip,data=nil,th_ack=0,th_dport,th_flags,
                      th_off,th_seq=0,th_sport,th_sum,th_urp,th_win,th_x2,update_ip_len){
    var payloadsz = 0;
    if(!ip){
        return nil;
    }
    if(data){
        payloadsz = len(data);
    }
    var ipsz = get_ipv4_hl(ip)*4;
    if(len(ip)<ipsz){
        return nil;
    }
    if(th_seq==0){
        th_seq = rand();
    }
    var tcp = build_tcp_header(th_sport,th_dport,th_seq,th_ack,th_flags,th_win,th_urp);
    ip = set_ipv4_length(ip,len(tcp)+payloadsz+ipsz);
    ip = update_ipv4_checksum(ip);
    tcp = update_tcp_checksum(ip,tcp,data);
    var total = append(bytes(ip),tcp);
    if(data){
         total = append(total,data);
    }
    return total;
}

func forge_tcp_v6_packet(ip,data=nil,th_ack=0,th_dport,th_flags,
                      th_off,th_seq=0,th_sport,th_sum,th_urp,th_win,th_x2,update_ip_len){
    var payloadsz = 0;
    if(!ip){
        return nil;
    }
    if(data){
        payloadsz = len(data);
    }
    var ipsz = get_ipv4_hl()*4;
    if(len(ip)<ipsz){
        return nil;
    }
    if(th_seq==0){
        th_seq = rand();
    }
    var tcp = build_tcp_header(th_sport,th_dport,th_seq,ack,th_flags,th_win,th_urp);
    ip = set_ipv6_plen(ip,len(tcp)+payloadsz);
    tcp = update_tcp_checksum(ip,tcp,data);
    var total = append(bytes(ip),tcp);
    if(data){
         total = append(total,data);
    }
    return total;
}

func get_tcp_element_from_tcp(tcp,element){
    switch(element){
        case "th_sport":{
            return ReadUInt16(tcp,0);
        }
        case "th_dsport":{
            return ReadUInt16(tcp,2);
        }
        case "th_seq":{
            return ReadUInt32(tcp,4);
        }
        case "th_ack":{
            return ReadUInt32(tcp,8);
        }
        case "th_x2":{
            return 0;
        }
        case "th_off":{
            return ToInteger((tcp[12]>>4)&0xFF);
        }
        case "th_flags":{
            return ToInteger((tcp[12]&0x3F)&0xFF);
        }
        case "th_win":{
            return ReadUInt16(tcp,14);
        }
        case "th_sum":{
            return ReadUInt16(tcp,16);
        }
        case "th_urp":{
            return ReadUInt16(tcp,18);
        }
    }
}

func get_tcp_element(tcp,element){
    if(!tcp){
        return nil;
    }
    if(tcp[0]>>4 != 4){
        return nil;
    }
    var ipsz = get_ipv4_hl(tcp)*4;
    if(ipsz > len(tcp)){
        return nil;
    }
    var ip_len = get_ipv4_length(tcp);
    if(ip_len >len(tcp)){
        return nil;
    }
    var hdr = tcp[ipsz:];
    var datasz = ip_len - ipsz - ToInteger((hdr[12]>>4)&0xFF)*4;
    var offset = ipsz+ToInteger((hdr[12]>>4)&0xFF)*4;
    if(element == "data"){
        return tcp[offset:offset+datasz];
    }
    return get_tcp_element_from_tcp(hdr,element);
}

func get_tcp_v6_element(tcp,element){
    if(!tcp){
        return nil;
    }
    if(tcp[0]>>4 != 6){
        return nil;
    }
    var ip_len = get_ipv6_plen(tcp);
    if(ip_len > len(tcp)){
        return nil;
    }
    var hdr = tcp[40:];
    if(element != "data"){
        return get_tcp_element_from_tcp(hdr,element);
    }
    var datasz = ip_len - ToInteger((hdr[12]>>4)&0xFF)*4;
    var offset = 40+ToInteger((hdr[12]>>4)&0xFF)*4;
    if(offset + datasz > len(tcp)){
        return nil;
    }
    return tcp[offset:offset+datasz];
}

func dump_tcp_packet(packet){
    var tcp = display_ip_layer(packet);
    Println(" ");
    Println("Src Port:\t"+ReadUInt16(tcp,0));
    Println("Dst Port:\t"+ReadUInt16(tcp,2));
    Println("Seq:\t\t"+ ReadUInt32(tcp,4));
    Println("ACK:\t\t"+ ReadUInt32(tcp,8));
    Println("OFF:\t\t"+ToInteger((tcp[12]>>4)&0xFF));
    Println("Flags:\t\t"+ToInteger((tcp[12]&0x3F)&0xFF));
    Println("WIN:\t\t"+ReadUInt16(tcp,14));
    Println("CheckSum:\t"+HexEncode(ReadUInt16(tcp,16)));
    Println("URP:\t\t"+ReadUInt16(tcp,18));
}

func dump_tcp_v6_packet(packet){
    dump_tcp_packet(packet);
}

#func build_udp(ip,src_port,dst_port,data)
func forge_udp_packet(ip, data, uh_dport=0, uh_sport=0, uh_sum,uh_ulen, update_ip_len){
    if(!ip){
        return nil;
    }
    var hdr = build_udp(ip,uh_sport,uh_dport,data);
    ip = set_ipv4_length(ip,len(hdr)+get_ipv4_hl(ip)*4);
    var total = append(bytes(ip),hdr);
    total = update_ipv4_checksum(total);
    return total;
}

func forge_udp_v6_packet(ip6, data, uh_dport=0, uh_sport=0, uh_sum,uh_ulen, update_ip_len){
    var ip = ip6;
    if(!ip){
        return nil;
    }
    var hdr = build_udp(ip,uh_sport,uh_dport,data);
    ip = set_ipv6_plen(ip,len(hdr));
    var total = append(bytes(ip),hdr);
    return total;
}


func get_udp_element(udp, element){
    if(!udp){
        return nil;
    }
    if(udp[0]>>4 != 4){
        return nil;
    }
    var ipsz = get_ipv4_hl(udp)*4;
    if(ipsz > len(udp)){
        return nil;
    }
    var ip_len = get_ipv4_length(udp);
    if(ip_len > len(udp)){
        return nil;
    }
    var hdr = udp[ipsz:];
    if(element == "data"){
        var offset = ipsz+8;
        var datasz = ReadUInt16(hdr,4);
        if(datasz +offset > len(udp)){
            datasz = len(udp)-offset;
        }
        return udp[offset:offset+datasz];
    }
    switch(element){
        case "uh_sport":{
            return ReadUInt16(hdr,0);
        }
        case "uh_dport":{
            return ReadUInt16(hdr,2);
        }
        case "uh_ulen":{
            return ReadUInt16(hdr,4);
        }
        case "uh_sum":{
            return ReadUInt16(hdr,6);
        }
    }
    return nil;

}

func get_udp_v6_element(udp,element){
    var ip = udp;
    if(!ip){
        return nil;
    }
    if(ip[0]>>4 != 6){
        return nil;
    }
    var ip_len = get_ipv6_plen(ip);
    if(ip_len > len(ip)){
        return nil;
    }
    var hdr = ip[40:];
    if(element == "data"){
        var offset = 48;
        var datasz = ReadUInt16(hdr,4);
        if(datasz +offset > len(ip)){
            datasz = len(ip)-offset;
        }
        return ip[offset:offset+datasz];
    }
    switch(element){
        case "uh_sport":{
            return ReadUInt16(hdr,0);
        }
        case "uh_dport":{
            return ReadUInt16(hdr,2);
        }
        case "uh_ulen":{
            return ReadUInt16(hdr,4);
        }
        case "uh_sum":{
            return ReadUInt16(hdr,6);
        }
    }
    return nil;

}

func dump_udp_packet(packet){
    var hdr = display_ip_layer(packet);
    if(hdr == nil){
        return;
    }
    if(len(hdr) < 8){
        return;
    }
    Println(" ");
    Println("Src Port:\t"+ReadUInt16(hdr,0));
    Println("Dst Port:\t"+ReadUInt16(hdr,2));
    Println("Length:\t"+ReadUInt16(hdr,4));
    Println("CheckSum:\t"+HexEncode(ReadUInt16(hdr,6)));
}

func dump_udp_v6_packet(packet){
    dump_udp_packet(packet);
}

func forge_icmp_packet(ip,data="",icmp_type,icmp_code,icmp_seq,icmp_id,icmp_cksum,update_ip_len=true){
    var ipsz = get_ipv4_hl(ip)*4;
    if(ipsz>len(ip)){
        return nil;
    }
    var icmp = MakeBytes(8+len(data));
    icmp[0] = icmp_type;
    icmp[1] = icmp_code;
    icmp = WriteUInt16(icmp,4,icmp_id);
    icmp = WriteUInt16(icmp,6,icmp_seq);
    if(data){
        CopyBytes(icmp[8:],data);
    }
    var sum = cacl_ip_checksum(icmp,len(icmp));
    icmp = WriteUInt16(icmp,2,sum,false);
    ip = set_ipv4_length(ip,len(icmp)+ipsz);
    ip = update_ipv4_checksum(ip);
    return append(bytes(ip),icmp);
}

func get_icmp_element(icmp,element){
    var ip = icmp;
    if(!ip){
        return nil;
    }
    if(ip[0]>>4 != 4){
        return nil;
    }
    var ipsz = get_ipv4_hl(ip)*4;
    if(ipsz > len(ip)){
        return nil;
    }
    var ip_len = get_ipv4_length(ip);
    if(ip_len > len(ip)){
        return nil;
    }
    var hdr = ip[ipsz:];
    if(element == "data"){
        var offset = ipsz+8;
        if(ipsz+8 < len(ip)){
            var size = len(ip)-ipsz - 8;
            return ip[offset:offset+size];
        }
        return nil;
    }
    switch(element){
        case "icmp_type":{
            return hdr[0];
        }
        case "icmp_code":{
            return hdr[1];
        }
        case "icmp_id":{
            return ReadUInt16(hdr,4);
        }
        case "icmp_seq":{
            return ReadUInt16(hdr,6);
        }
        case "icmp_cksum":{
            return ReadUInt16(hdr,2);
        }
    }
    return nil;
}

func forge_icmp_v6_packet(ip6,data="",icmp_type=0,icmp_code=0,icmp_id=0,icmp_seq=0,
                        reachable_time,retransmit_timer,flags,target,icmp_cksum,update_ip_len=true){
    if(!ip6 || len(ip6) < 40){
        return nil;
    }
    var ipsz =len(ip6);
    var hdr = MakeBytes(32);
    var hdr_size = 8;
    hdr[0] = icmp_type;
    hdr[1] = icmp_code;
    switch (icmp_type){
        case 128:{
            hdr = WriteUInt16(hdr,4,icmp_id);
            hdr = WriteUInt16(hdr,6,icmp_seq);
        }
        case 134:{
            hdr[4] = ip6[7];
            hdr[5] = byte(flags);
            hdr = WriteUInt32(hdr,8,reachable_time);
            hdr = WriteUInt32(hdr,12,retransmit_timer);
            hdr_size = 16;
        }
        case 135:{
            CopyBytes(hdr[8:],ip6[24:40]);
            hdr_size = 24;
        }
        case 136:{
            hdr = WriteUInt32(hdr,4,flags);
            flags = ReadUInt32(hdr,4);
            if(flags & 0x00000020){
                CopyBytes(hdr[8:],ip6[8:24]);
            }else{
                CopyBytes(hdr[8:],ipv6_string_to_address(target));
            }
            hdr_size = 24;
        }
        default:{
            hdr_size = 8;
        }
    }
    var full = hdr[:hdr_size];
    if(data != nil){
        full = append(full,bytes(data));
    }
    ip6 = set_ipv6_plen(ip6,len(full));
    var ps_head = MakeBytes(36);
    ps_head[33] = 0x3a;
    ps_head = WriteUInt16(ps_head,34,len(full));
    CopyBytes(ps_head,ip6[8:24]);
    CopyBytes(ps_head[16:],ip6[24:40]);
    var sum = cacl_ip_checksum(append(ps_head,full),36+len(full));
    full = WriteUInt16(full,2,sum,false);
    return append(bytes(ip6),full);
}


func get_icmp_v6_element(icmp,element){
    var ip = icmp;
    if(!ip){
        return nil;
    }
    if(ip[0]>>4 != 6){
        return nil;
    }
    if(len(ip) < 48){
        return nil;
    }
    if (get_ipv6_plen(ip) < 8){
        return nil;
    }
    if (get_ipv6_plen(ip)+40 < len(ip)){
        return nil;
    }
    var hdr = ip[40:];
    if(element == "data"){
        var offset = ipsz+8;
        if(48 < len(ip)){
            var size = len(ip)-48;
            return ip[offset:offset+size];
        }
        return nil;
    }
    switch(element){
        case "icmp_type":{
            return hdr[0];
        }
        case "icmp_code":{
            return hdr[1];
        }
        case "icmp_id":{
            return ReadUInt16(hdr,4);
        }
        case "icmp_seq":{
            return ReadUInt16(hdr,6);
        }
        case "icmp_cksum":{
            return ReadUInt16(hdr,2);
        }
    }
    return nil;
}

func dump_icmp_packet(packet){
    Println(HexDumpBytes(packet));
}

func dump_icmp_v6_packet(packet){
    Println(HexDumpBytes(packet));
}

func forge_igmp_packet(){

}

func forge_igmp_v6_packet(){

}

func send_packet(packet,length,pcap_active=true,pcap_filter="",pcap_timeout=5,allow_broadcast){
    return PcapSend(packet,pcap_filter,pcap_timeout,pcap_active);
}

func send_v6packet(packet,length,pcap_active=true,pcap_filter="",pcap_timeout=5,allow_broadcast){
    return PcapSend(packet,pcap_filter,pcap_timeout,pcap_active);
}

func pcap_next(interface="",pcap_filter="",timeout=5){
    return CapturePacket(interface,pcap_filter,timeout);
}

replace_kb_item("http/user-agent","Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36");