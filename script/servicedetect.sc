#runtime 

var detect_port = 52204;
var _host_name = "127.0.0.1";
var _http_req = "GET / HTTP/1.0\r\nHost: "+_host_name+"\r\n\r\n";


func regisger_service(name,banner){
    Println("detect ",name,ToString(banner));
}

func grab_banner(req){
    var soc = TCPConnect(_host_name,detect_port,15,false);
    if(soc == nil){
        return nil;
    }
    if (req != nil){
        ConnWrite(soc,req);
    }
    ConnSetReadTimeout(soc,5);
    return ConnRead(soc,16*1024);
}

func first_line(buf){
    var index = IndexString(buf,"\n");
    if(index != -1){
        return buf[:index];
    }
    return clone(buf);
}

func ReadUInt32FromBuffer(buf,isBigEndian){
    var val ;
    if(isBigEndian){
        val = ((buf[0]&0xFF)<<24)|((buf[1]&0xFF)<<16) |((buf[2]&0xFF)<<8) |buf[3]&0xFF;
    }else{
        val = ((buf[3]&0xFF)<<24)|((buf[2]&0xFF)<<16) |((buf[1]&0xFF)<<8) |buf[0]&0xFF;
    }
    return val &0xFFFFFFFF;
}

func ABS(x){
    if(x < 0){
        return -x;
    }
    return x;
}

func may_be_time(banner){
    var nValue = ReadUInt32FromBuffer(banner,true);
    var nTime = unixtime();
    var rt70 = nValue - 2208988800;
    if(ABS(nTime - rt70) < (3 * 365 * 86400)){
        return true;
    }
    return false;
}

func mark_pop_server(banner){
    var low = ToLowerString(banner);
    if(low == "+ok"){
        regisger_service("pop1",banner);
    }else if(ContainsBytes(low,"pop2")){
        regisger_service("pop2",banner);
    }else{
        regisger_service("pop3",banner);
    }
}

func detect_banner(banner){
    var buf = ToLowerString(banner);
    var line = first_line(buf);
    var origline = first_line(banner);


    if (HasPrefixString(line,"http/1.") || ContainsBytes(banner,"<title>Not supported</title>")){
        if(!(detect_port ==5000 && ContainsBytes(line,"http/1.1 400 bad request"))&&
           !(HasPrefixString(line,"http/1.0 403 forbidden")&&ContainsBytes(buf,"server: adsubtract"))&&
           !(ContainsBytes(buf,"it looks like you are trying to access mongodb over http on the native driver port.")&&ContainsBytes(buf,"content-length: 84"))){
               regisger_service("www",banner);
           }
    }

    if (len(buf) >= 2 && (buf[0]== 255 &&(buf[1]==251 ||buf[1]==252 || buf[1]==253 || buf[1]==254 ) )){
        regisger_service("telnet",banner);
        return;
    }
    if (len(buf) >= 4 && (buf[0]== 0 && buf[1] == 1 && buf[2] == 1 && buf[3] == 0)){
        regisger_service("gnome14",banner);
        return;
    }
    if (HasPrefixString(buf,"http/1.0 403 forbidden") && ContainsBytes(buf,"server: adsubtract")){
        regisger_service("AdSubtract",banner);
        return;
    }
    if (ContainsBytes(banner,"Eggdrop")&&ContainsBytes(banner,"Eggheads")){
        regisger_service("eggdrop",banner);
        return;
    }
    if (HasPrefixString(line,"$lock ")){
        regisger_service("DirectConnectHub",banner);
        return;
    }
    if (len(buf) > 34 && ContainsBytes(buf[34:],"iss ecnra")){
        regisger_service("issrealsecure",banner);
        return;
    }
    if (len(banner) == 4 && origline[0] == 'Q' && origline[1] == 0 && origline[2] == 0 && origline[3] == 0 ){
        regisger_service("cpfw1",banner);
        return;
    } 
    if (ContainsBytes(line,"adsgone blocked html ad")){
        regisger_service("adsgone",banner);
        return;
    }
    if (HasPrefixString(line,"icy 200 ok")){
        regisger_service("shoutcast",banner);
        return;
    }
    if (HasPrefixString(line,"icy 200 ok")){
        regisger_service("shoutcast",banner);
        return;
    }
    if (HasPrefixString(line,"200") && 
        (ContainsBytes(line,"running eudora internet mail server") 
        || ContainsBytes(line,"+ok applepasswordserver"))){
        regisger_service("pop3pw",banner);
        return;
    }
    if (HasPrefixString(line,"220")&&(
        ContainsBytes(line,"smtp") ||
        ContainsBytes(line,"simple mail transfer")||
        ContainsBytes(line,"mail server")||
        ContainsBytes(line,"messaging")||
        ContainsBytes(line,"Weasel")
    )){
        regisger_service("smtp",banner);
        return;
    }
    if (ContainsBytes(line,"220 ***************") || ContainsBytes(line,"220 eSafe@")){
        regisger_service("smtp",banner);
        return;
    }
    if (ContainsBytes(line,"220 esafealert")){
        regisger_service("smtp",banner);
        return;
    }
    if (HasPrefixString(line,"220") && 
        (ContainsBytes(line,"groupwise internet agent"))){
        regisger_service("smtp",banner);
        return;
    }
    if (HasPrefixString(line,"220") && 
        (ContainsBytes(origline," SNPP "))){
        regisger_service("smtp",banner);
        return;
    }
    if (HasPrefixString(line,"200") && 
        (ContainsBytes(line,"mail "))){
        regisger_service("smtp",banner);
        return;
    }
    if (HasPrefixString(line,"421") && 
        (ContainsBytes(line,"smtp "))){
        regisger_service("smtp",banner);
        return;
    }
    if ((line[0] != 0 || 
        ContainsBytes(line,"mysql")) && 
        (IsMatchRegexp(buf,"^.x{3}\n[0-9.]+ [0-9a-z]+@[0-9a-z]+ release")||
        IsMatchRegexp(buf,"^.x{3}\n[0-9.]+-(id[0-9]+-)?release \\([0-9a-z-]+\\)"))){
        regisger_service("sphinxql",banner);
        return;
    }
    if (line[0] != 0 && (
        HasPrefixString(buf[1:],"host '") ||
        ContainsBytes(buf,"mysql") ||
        ContainsBytes(buf,"mariadb")
    )){
        regisger_service("mysql",banner);
        return;
    }
    if (HasPrefixString(line,"efatal") || HasPrefixString(line,"einvalid packet length")){
        regisger_service("postgresql",banner);
        return;
    }
    if (ContainsBytes(line,"cvsup server ready")){
        #cvsup
        regisger_service("cvsup",banner);
        return;
    }

    if (HasPrefixString(line,"cvs [pserver aborted]:") || HasPrefixString(line,"cvs [server aborted]:")){
        regisger_service("cvspserver",banner);
        return;
    }

    if (HasPrefixString(line,"cvslock ")){
        regisger_service("cvslockserver",banner);
        return;
    }

    if (HasPrefixString(line,"@rsyncd")){
        regisger_service("rsync",banner);
        return;
    }

    if (len(banner) == 0 && may_be_time(banner)){
        regisger_service("time",banner);
        return;
    }

    if(ContainsBytes(buf,"rmserver") || ContainsBytes(buf,"realserver")){
        regisger_service("realserver",banner);
        return;
    }
    if ((ContainsBytes(line,"ftp")||
        ContainsBytes(line,"winsock")||
        ContainsBytes(line,"axis network camera")||
        ContainsBytes(line,"netpresenz")||
        ContainsBytes(line,"serv-u")||
        ContainsBytes(line,"service ready for new user"))&&
        !HasPrefixString(line,"220")){
        regisger_service("ftp",banner);
        return;
    }
    if (HasPrefixString(line,"220-")){
        regisger_service("ftp",banner);
        return;
    }
    if (ContainsBytes(line,"220") && ContainsBytes(line,"whois+")){
        regisger_service("whois++",banner);
        return;
    }
    if (ContainsBytes(line,"520 command could not be executed")){
        regisger_service("mon",banner);
        return;
    }
    
    if (ContainsBytes(line,"ssh-")){
        regisger_service("ssh",banner);
        return;
    }
    if (HasPrefixString(line,"+ok") ||
        (line[0]=='+' && ContainsBytes(line,"pop"))){
        mark_pop_server(origline);
        return;
    }
    if (ContainsBytes(line,"imap4") && HasPrefixString(line,"* ok")){
        regisger_service("imap",banner);
        return;
    }
    if (ContainsBytes(line,"*ok iplanet messaging multiplexor")){
        regisger_service("imap",banner);
        return;
    }

    if (ContainsBytes(line,"*ok communigate pro imap server")){
        regisger_service("imap",banner);
        return;
    }

    if (ContainsBytes(line,"* ok courier-imap")){
        regisger_service("imap",banner);
        return;
    }

    if (HasPrefixString(line,"giop")){
        regisger_service("giop",banner);
        return;
    }

    if (ContainsBytes(line,"microsoft routing server")){
        regisger_service("exchg-routing",banner);
        return;
    }
    if (ContainsBytes(line,"gap service ready")){
        regisger_service("iPlanetENS",banner);
        return;
    }
    if (ContainsBytes(line,"-service not available")){
        regisger_service("tcpmux",banner);
        return;
    }
    if (len(line)> 2 && line[0]== 0x7f && line[1]==0x7f && HasPrefixString(line[2:],"ica")){
        regisger_service("citrix",banner);
        return;
    }
    if (ContainsBytes(origline," INN ")||
        ContainsBytes(origline," Leafnode ")||
        ContainsBytes(line,"  nntp daemon")||
        ContainsBytes(line," nnrp service ready")||
        ContainsBytes(line,"posting ok")||
        ContainsBytes(line,"posting allowed")||
        ContainsBytes(line,"502 no permission")||
        (HasPrefixString(line,"502")&&ContainsBytes(line,"diablo"))){
        regisger_service("nntp",banner);
        return;
    }
    if (ContainsBytes(buf,"networking/linuxconf")||
        ContainsBytes(buf,"networking/misc/linuxconf")||
        ContainsBytes(buf,"server: linuxconf")){
        regisger_service("linuxconf",banner);
        return;
    }
    if (HasPrefixString( buf,"gnudoit:")){
        regisger_service("gnuserv",banner);
        return;
    }
    if ((buf[0]==0 && ContainsBytes(buf,"error.host\t1")) ||
        (buf[0]==3 && ContainsBytes(buf,"That item is not currently available"))){
        regisger_service("gopher",banner);
        return;
    }
    if(ContainsBytes(buf,"www-authenticate: basic realm=\"swat\"")){
        regisger_service("swat",banner);
        return;
    }
    if(ContainsBytes(buf,"vqserver") &&
        ContainsBytes(buf,"www-authenticate: basic realm=/")){
        regisger_service("vqServer-admin",banner);
        return;
    }
    if(ContainsBytes(buf,"1invalid request")){
        regisger_service("mldonkey",banner);
        return;
    }
    if (ContainsBytes(buf,"get: command not found")){
        regisger_service("wild_shell",banner);
        return;
    }

    if (ContainsBytes(buf,"microsoft windows")&&
        ContainsBytes(buf,"c:\\")&&
        ContainsBytes(buf,"(c) copyright 1985-")&&
        ContainsBytes(buf,"microsoft corp.")){
        regisger_service("wild_shell",banner);
        return;
    }

    if (ContainsBytes(buf,"netbus")){
        regisger_service("netbus",banner);
        return;
    }

    if (ContainsBytes(buf,"netbus")){
        regisger_service("netbus",banner);
        return;
    }

    if (ContainsBytes(line,"0 , 0 : error : unknown-error")||
        ContainsBytes(line,"0, 0: error: unknown-error")||
        ContainsBytes(line,"get : error : unknown-error")||
        ContainsBytes(line,"0 , 0 : error : invalid-port")){
        regisger_service("auth",banner);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"proxy")){
        regisger_service("http_proxy",banner);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"via: ")){
        regisger_service("http_proxy",banner);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"proxy-connection: ")){
        regisger_service("http_proxy",banner);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"cache")&&
        ContainsBytes(line,"bad request")){
        regisger_service("http_proxy",banner);
        return;
    }
    if (HasPrefixString(origline,"RFB 00")&&
        ContainsBytes(line,".00")){
        regisger_service("vnc",banner);
        return;
    }
    if (HasPrefixString(line,"ncacn_http/1.")){
        if(detect_port == 593){
            regisger_service("http-rpc-epmap",banner);
        }else{
            regisger_service("ncacn_http");
        }
        return;
    }
    if (len(line)>=14 && len(line)<=18 && HasPrefixString(_http_req,origline)){
        regisger_service("echo",banner);
        return;
    }
    if (ContainsBytes(banner,"!\"#$%&'()*+,-./")&&
        ContainsBytes(banner,"ABCDEFGHIJ")&&
        ContainsBytes(banner,"abcdefghij"&&
        ContainsBytes(banner,"0123456789"))){
        regisger_service("chargen",banner);
        return;
    }
    if (ContainsBytes(line,"vtun server")){
        regisger_service("vtun",banner);
        return;
    }
    if (line=="login: password: "){
        regisger_service("uucp",banner);
        return;
    }
    if (line=="bad request" ||
        ContainsBytes(line,"invalid protocol request (71): gget / http/1.0")||
        HasPrefixString(line,"lpd:")||
        ContainsBytes(line,"lpsched")||
        ContainsBytes(line,"malformed from address")||
        ContainsBytes(line,"no connect permissions")){
        regisger_service("ldp",banner);
        return;
    }
    if(ContainsBytes(line,"%%lyskom unsupported protocol")){
        regisger_service("lyskom",banner);
        return;
    }
    if(ContainsBytes(line,"598:get:command not recognized")){
        regisger_service("ph",banner);
        return;
    }
    if(ContainsBytes(origline,"BitTorrent prot")){
        regisger_service("BitTorrent",banner);
        return;
    }
    if(ContainsBytes(origline,"BitTorrent prot")){
        regisger_service("BitTorrent",banner);
        return;
    }
    if (len(banner)>=4 && banner[0] =='A' && banner[1] == 1 && banner[2]== 2 && banner[3] ==0){
        regisger_service("smux",banner);
        return;
    }
    if (HasPrefixString(buf,"0 succeeded\n")){
        regisger_service("LISa",banner);
        return;
    }
    if (HasPrefixString(buf,"0 succeeded\n")){
        regisger_service("LISa",banner);
        return;
    }
    if (len(banner) == 3 && banner[2]=='\n'){
        regisger_service("msdtc",banner);
        return;
    }
    if (HasPrefixString(line,"220") &&
        ContainsBytes(line,"poppassd")){
        regisger_service("pop3pw",banner);
        return;
    }
    if (ContainsBytes(line,"welcome!psybnc@")){
        regisger_service("psybnc",banner);
        return;
    }
    if (HasPrefixString(line,"* acap ")){
        regisger_service("acap",banner);
        return;
    }
    if (ContainsBytes(origline,"Sorry, you (")&&
        ContainsBytes(origline,"are not among the allowed hosts...")){
        regisger_service("nagiosd",banner);
        return;
    }
}

var banner = grab_banner(nil);
if(banner){
    detect_banner(banner);
}else{
    banner = grab_banner(_http_req);
    if(banner){
        detect_banner(banner);
    }
}

Println(banner);