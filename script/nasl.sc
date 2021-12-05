#nasl 内置的全局变量
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

#nasl一些特色的函数，我们有更好的实现，这里做转接


func add_to_array(array,val){
	var type = typeof(val);
	switch(type){
		case "array","map":{
			for v in val{
				array = add_to_array(array,v);
			}	
		}
		default:{
			array = append(array,val);
		}
	}
	return array;
}

#
#由于新版数组不能使用下标来索引，为保持兼容性，返回一个map
func make_list(list...){
	if(len(list)==0){
		return {};
	}
	var ret = [];
	for v in list{
		ret = add_to_array(ret,v);
	}
	return ret;
}

func power(x,y){
	for( i =0;i<y;i++){
		x*=x;
	}
	return x;
}

func make_array(list...){
	var result = {};
	var size = len(list);
	if(size > 0){
		if(size %2){
			error("make_array: the count of parameters not algin to 2 <count="+ToString(size)+">");
		}
		for(i=0;i<size-1;i+=2){
			result[list[i]] = list[i+1];
		}
	}
	return result;
}

func NASLTypeof(name){
	var result = typeof(name);
	if (result == "map"){
		return "array";
	}
	return result;
}

#为了方便维护，所有内置函数不能通过命名参数来调用
#所以原有的命名函数这里这里做中转
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

func http_get(item, port){
	var header ={"Connection":"close"};
    header["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9";
    header["Accept-Encoding"] = "gzip, deflate";
    header["Accept-Language"] = "zh-CN,zh,en;q=0.9,en;q=0.8";
    header["User-Agent"]="Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36";
	if(port != 80 && port != 443){
		header["Host"] = get_host_name()+":"+port;
	}else{
		header["Host"] = get_host_name();
	}
	header["Accept-Charset"] = "iso-8859-1,*,utf-8";
	header["Pragma"] = "no-cache";
	header["Cache-Control"] = "no-cache";
	if(!item){
		item = "/";
	}
	item = URLPathEscape(item);
	var result = "GET "+item + " HTTP/1.1\r\n";
	for k,v in header{
		result += k;
		result += ": ";
		result += v;
		result += "\r\n";
	}
	result += "\r\n";
	return result;
}


func send(socket,data){
	return ova_send(socket,data);
}

func recv_line(socket,length){
	var ret = ConnReadUntil(socket,"\r\n",length);
	if(!ret){
		return "";
	}
	return ret;
}


func recv(socket,length,timeout,min){
	return ova_recv(socket,length,timeout,min);
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
func open_sock_tcp(port,buffsz,timeout, transport=0,priority=0){
	if(!timeout){
		timeout = 30;
	}
	return TCPConnect(get_host_ip(),ToInteger(port),timeout,transport>0);
}

#match(string: r, pattern: "\x01rlogind: Permission denied*", icase: TRUE)
func match(string,pattern,icase=false){
	return ova_match(string,pattern,icase);
}

#pread(cmd: "nmap", argv: make_list( "nmap",open_sock_tcp(port, transport: ENCAPS_IP)
func pread(cmd,argv){
	error("nasl:pread not implement");
}

#wmi_query(wmi_handle: handle, query: query)
func wmi_query(wmi_handle,query){
	error("nasl:wmi_query not implement");
}

#wmi_connect(host: host, username: usrname, password: passwd)
func wmi_connect(host,username, password){
	error("nasl:wmi_connect not implement");
}

#wmi_close(wmi_handle: handle)
func wmi_close(wmi_handle){
	error("nasl:wmi_close not implement");
}

#http_post(port: port, item: "/tkset/systemstatus", data: "")
func http_post(port,item,data){
	error("nasl:http_post not implement");
}

#ssh_connect(socket: soc)
func ssh_connect(socket){
	error("nasl:ssh_connect not implement");
}

#ssh_userauth(session, login: NULL, password: NULL, privatekey: NULL, passphrase: NULL)
func ssh_userauth(session,login,password, privatekey, passphrase){
	error("nasl:ssh_userauth not implement");
}

#ssh_shell_write(sess, cmd: user + "\n" + pass + "\n" + "show sysinfo\n\nshow inventory\n")
func ssh_shell_write(session, cmd){
	error("nasl:ssh_shell_write not implement");
}

#ssh_request_exec(sess, cmd: cmd, stdout: 1, stderr: 1)
func ssh_request_exec(session, cmd,stdout,stderr){
	error("nasl:ssh_request_exec not implement");
}

#ssh_shell_open(sess, pty: TRUE)
func ssh_shell_open(session,pty){
	error("nasl:ssh_shell_open not implement");
}

#open_priv_sock_udp(dport: port, sport: port)
func open_priv_sock_udp(dport, sport){
	error("nasl:open_priv_sock_udp not implement");
}

#win_cmd_exec(cmd: command, password: password, username: username)
func win_cmd_exec(cmd, password, username){
	error("nasl:win_cmd_exec not implement");
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
	error("nasl:socket_negotiate_ssl not implement");
}

#gunzip(data: body)
func gunzip(data){
	return DeflateBytes(data);
}


func string(x...){
	var ret="";
	for v in x {
		if(v==nil){
			continue;
		}
		ret += v;
	}
	return ret;
}

func raw_string(x...){
	return bytes(x);
}

func isnull(val){
	return (val==nil);
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
	if(obj > 255){
		DisplayContext();
		error("debug me");
	}
	return obj;
}

func hex(val){
	 var hexText = HexEncode(val);
	 if(len(hexText)%2){
		 return "0x0"+hexText;
	 }
	return "0x"+hexText;
}

func hexstr(str){
	return HexEncode(str);
}

func strstr(str1,str2){
	var pos = IndexString(str1,str2);
	if(pos == -1){
		return nil;
	}
	return str1[pos:];
}

func substr(str,start,end=0){
	if(end == start){
		return str[start:start+1];
	}
	if(end<start){
		return str[start:];
	}
	return str[start:end];
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
	return ToLowerString(str);
}

func toupper(str){
	return ToUpperString(str);
}

func crap(length,data){
	return RepeatString(data,length);
}

func strlen(str){
	if(str==nil){
		return 0;
	}
	return len(str);
}

func egrep(pattern, string,icase=false){
	var text_group = SplitString(string,"\n");
	var result = "";
	for v in text_group {
		if(len(v)> 0 && IsMatchRegexp(v,pattern,icase)){
			result +=v;
			result +="\n";
		}
	}
	return ToString(result);
}

func ereg(pattern, string,multiline=false,icase=false){
	var text = string;
	if(!multiline){
		var lines = SplitString(string,"\n");
		text = lines[0];
	}
	return IsMatchRegexp(text,pattern,icase);
}

func ereg_replace(pattern, string,replace,icase=false){
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
	var result = SearchRegExp(string,pattern,icase);
	if(result != nil && len(result)==0){
		return nil;
	}
	return result;
}

func split(buffer, sep="\n",keep = false){
	var list = SplitString(buffer,sep);
	if(keep){
		var i = 0;
		for(var i = 0; i < len(list);i++){
			list[i]+=sep;
		}
	}
	return list;
}

func chomp(str){
	return TrimRightString(str," \t\n\r");
}

func int(other){
	return ToInteger(other);
}

func stridx(str,sub){
	return IndexString(str,sub);
}

func str_replace(string, find, replace,count = -1){
	return ReplaceString(string,find,replace,count);
}

func keys(obj){
	var list = [];
	if(typeof(obj)=="array"){
		DisplayContext();
		Println("please check this may be have some bug**************");
	}
	for k,v in obj{
		list = append(list,k);
	}
	return list;
}

func max_index(obj){
	if(typeof(obj)=="array"){
		return len(obj);
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
	if(port==443){
		return ENCAPS_TLSv12;
	}
	if(port==80){
		return 0;
	}
	return get_kb_item("Transports/TCP/"+port);
}

func http_open_socket(port){
	var ip = get_host_ip();
	var transport = get_port_transport(port);
	var isTLS = false;
	if(transport == nil){
		if(port == 443){
			isTLS = true;
		}
	}else{
		isTLS = (transport > 0);
	}
	return TCPConnect(ip,port,30,isTLS);
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

replace_kb_item("http/user-agent","Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36");