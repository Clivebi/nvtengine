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
	#Println(name,"-->",value);
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

#forge_ip_packet(ip_hl: 5, ip_v: 4, ip_off: 0, ip_id: 9, ip_tos: 0, ip_p: IPPROTO_ICMP, ip_len: 20, ip_src: host, ip_ttl: 255)
func forge_ip_packet(ip_hl, ip_v, ip_off, ip_id, ip_tos, ip_p, ip_len, ip_src, ip_ttl){
	error("nasl:forge_ip_packet not implement");
}

#forge_icmp_packet(ip: ip, icmp_type: 13, icmp_code: 0, icmp_seq: 1, icmp_id: 1)
func forge_icmp_packet(ip, icmp_type, icmp_code,icmp_seq, icmp_id){
	error("nasl:forge_icmp_packet not implement");
}

#send_packet(icmp, pcap_active: TRUE, pcap_filter: filter, pcap_timeout: 1)
#send_packet(udp_pkt, pcap_active: TRUE, pcap_filter: filter, allow_broadcast: TRUE)
func send_packet(packet, pcap_active,pcap_filter,allow_broadcast, pcap_timeout){
	error("nasl:send_packet not implement");
}
func send_v6packet(packet, pcap_active,pcap_filter,allow_broadcast, pcap_timeout){
	error("nasl:send_v6packet not implement");
}
#get_icmp_element(icmp: res, element: "icmp_type")
func get_icmp_element(icmp, element){
	error("nasl:get_icmp_element not implement");
}

#forge_udp_packet(ip: ip_pkt, uh_sport: srcport, uh_dport: dstport, uh_ulen: req_len, data: req)
func forge_udp_packet(ip, uh_sport,uh_dport, uh_ulen, data){
	error("nasl:forge_udp_packet not implement");
}

#get_udp_element(udp: res, element: "data")
func get_udp_element(udp, element){
	error("nasl:get_udp_element not implement");
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

func _filter_item(item,keys){
    var ret = {};
    for v in keys{
        ret[v] = item[v];
    }
    ret["Properties"] = item["Properties"];
    return ret;
}

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
    var list = SplitString(query,",");
    var keys =[];
    for v in list{
        keys = append(keys,TrimString(v," \t"));
    }
    return keys;
}

# "root\rsop\computer"
func WMIQuery(handle,query,namespace=nil){
    var cmd = "Get-WmiObject -Query {"+query+"}"+"| Format-List -Property *";
    if(namespace){
        cmd = "Get-WmiObject -Query {"+query+"}"+" -Namespace "+namespace+" | Format-List -Property *";
    }
    var result = WinRMCommand(handle,cmd,"",true);
    var temp = [];
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
                temp = append(temp,item);
                item = {};
            }
            continue;
        }
        if (HasPrefixString(key,"__")){
            continue;
        }
        item[key] = value;
    }
    temp = append(temp,item);
    var ret = [];
    for v in temp{
        var pros = TrimString(v["Properties"],"{}");
        if(!ContainsBytes(pros,"...")){
            ret = append(ret,_filter_item(v,SplitString(pros,", ")));
        }else{
            ret = append(ret,v);
        }
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
    if(len(result)==0){
        return "";
    }
    var pros = TrimString(result[0]["Properties"],"{}");
    var keys = [];
    if(!ContainsBytes(pros,"...")){
        keys = _split_keys_from_query_string(query);
    }else{
        if (len(result)) {
            for k,v in result[0]{
                keys = append(keys,k);
            }
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

replace_kb_item("http/user-agent","Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36");