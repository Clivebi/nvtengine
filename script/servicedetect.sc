#service detect script 

var _nasl_oid = "1.3.6.1.4.1.25623.1.0.10330";

func regisger_service(name,banner,port){
    replace_kb_item("HostDetails/NVT/"+_nasl_oid,"Service detection(1.3.6.1.4.1.25623.1.0.10330)");
    set_kb_item("HostDetails/NVT/"+_nasl_oid+"/Services",port+",tcp,"+name);
    set_kb_item("Services/"+name,port);
    replace_kb_item("Known/tcp/"+port,name);
    replace_kb_item(name+"/banner/"+port,banner);
    Println("Find a ",name," runing on the port ",ToString(port));
    #Println(banner);
}

func grab_banner(host,port,useTLS=false,req=nil){
    var soc = TCPConnect(host,port,15,useTLS);
    if(soc == nil){
        return nil;
    }
    if(useTLS){
        set_kb_item("Transport/SSL",port);
        set_kb_item("Transports/TCP/"+port,2);
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

func mark_pop_server(banner,port){
    var low = ToLowerString(banner);
    if(low == "+ok"){
        regisger_service("pop1",banner,port);
    }else if(ContainsBytes(low,"pop2")){
        regisger_service("pop2",banner,port);
    }else{
        regisger_service("pop3",banner,port);
    }
}

func detect_banner(banner,port){
    var buf = ToLowerString(banner);
    var line = first_line(buf);
    var origline = first_line(banner);
    var foundWWW = false;
    for (var i = 0; i < len(buf);i++){
        if(buf[i] == 0){
            buf[i] = 'x';
        }
    }


    if (HasPrefixString(line,"http/1.") || ContainsBytes(banner,"<title>Not supported</title>")){
        if(!(port ==5000 && ContainsBytes(line,"http/1.1 400 bad request"))&&
           !(HasPrefixString(line,"http/1.0 403 forbidden")&&ContainsBytes(buf,"server: adsubtract"))&&
           !(ContainsBytes(buf,"it looks like you are trying to access mongodb over http on the native driver port.")&&ContainsBytes(buf,"content-length: 84"))){
               regisger_service("www",banner,port);
               foundWWW = true;
           }
    }

    if (len(buf) >= 2 && (buf[0]== 255 &&(buf[1]==251 ||buf[1]==252 || buf[1]==253 || buf[1]==254 ) )){
        regisger_service("telnet",banner,port);
        return;
    }
    if (len(buf) >= 4 && (buf[0]== 0 && buf[1] == 1 && buf[2] == 1 && buf[3] == 0)){
        regisger_service("gnome14",banner,port);
        return;
    }
    if (HasPrefixString(buf,"http/1.0 403 forbidden") && ContainsBytes(buf,"server: adsubtract")){
        regisger_service("AdSubtract",banner,port);
        return;
    }
    if (ContainsBytes(banner,"Eggdrop")&&ContainsBytes(banner,"Eggheads")){
        regisger_service("eggdrop",banner,port);
        return;
    }
    if (HasPrefixString(line,"$lock ")){
        regisger_service("DirectConnectHub",banner,port);
        return;
    }
    if (len(buf) > 34 && ContainsBytes(buf[34:],"iss ecnra")){
        regisger_service("issrealsecure",banner,port);
        return;
    }
    if (len(banner) == 4 && origline[0] == 'Q' && origline[1] == 0 && origline[2] == 0 && origline[3] == 0 ){
        regisger_service("cpfw1",banner,port);
        return;
    } 
    if (ContainsBytes(line,"adsgone blocked html ad")){
        regisger_service("adsgone",banner,port);
        return;
    }
    if (HasPrefixString(line,"icy 200 ok")){
        regisger_service("shoutcast",banner,port);
        return;
    }
    if (HasPrefixString(line,"icy 200 ok")){
        regisger_service("shoutcast",banner,port);
        return;
    }
    if (HasPrefixString(line,"200") && 
        (ContainsBytes(line,"running eudora internet mail server") 
        || ContainsBytes(line,"+ok applepasswordserver"))){
        regisger_service("pop3pw",banner,port);
        return;
    }
    if (HasPrefixString(line,"220")&&(
        ContainsBytes(line,"smtp") ||
        ContainsBytes(line,"simple mail transfer")||
        ContainsBytes(line,"mail server")||
        ContainsBytes(line,"messaging")||
        ContainsBytes(line,"Weasel")
    )){
        regisger_service("smtp",banner,port);
        return;
    }
    if (ContainsBytes(line,"220 ***************") || ContainsBytes(line,"220 eSafe@")){
        regisger_service("smtp",banner,port);
        return;
    }
    if (ContainsBytes(line,"220 esafealert")){
        regisger_service("smtp",banner,port);
        return;
    }
    if (HasPrefixString(line,"220") && 
        (ContainsBytes(line,"groupwise internet agent"))){
        regisger_service("smtp",banner,port);
        return;
    }
    if (HasPrefixString(line,"220") && 
        (ContainsBytes(origline," SNPP "))){
        regisger_service("smtp",banner,port);
        return;
    }
    if (HasPrefixString(line,"200") && 
        (ContainsBytes(line,"mail "))){
        regisger_service("smtp",banner,port);
        return;
    }
    if (HasPrefixString(line,"421") && 
        (ContainsBytes(line,"smtp "))){
        regisger_service("smtp",banner,port);
        return;
    }
    if ((line[0] != 0 || 
        ContainsBytes(line,"mysql")) && 
        (IsMatchRegexp(buf,"^.x{3}\n[0-9.]+ [0-9a-z]+@[0-9a-z]+ release")||
        IsMatchRegexp(buf,"^.x{3}\n[0-9.]+-(id[0-9]+-)?release \\([0-9a-z-]+\\)"))){
        regisger_service("sphinxql",banner,port);
        return;
    }
    if (line[0] != 0 && (
        HasPrefixString(buf[1:],"host '") ||
        ContainsBytes(buf,"mysql") ||
        ContainsBytes(buf,"mariadb")
    )){
        regisger_service("mysql",banner,port);
        return;
    }
    if (HasPrefixString(line,"efatal") || HasPrefixString(line,"einvalid packet length")){
        regisger_service("postgresql",banner,port);
        return;
    }
    if (ContainsBytes(line,"cvsup server ready")){
        #cvsup
        regisger_service("cvsup",banner,port);
        return;
    }

    if (HasPrefixString(line,"cvs [pserver aborted]:") || HasPrefixString(line,"cvs [server aborted]:")){
        regisger_service("cvspserver",banner,port);
        return;
    }

    if (HasPrefixString(line,"cvslock ")){
        regisger_service("cvslockserver",banner,port);
        return;
    }

    if (HasPrefixString(line,"@rsyncd")){
        regisger_service("rsync",banner,port);
        return;
    }

    if (len(banner) == 0 && may_be_time(banner)){
        regisger_service("time",banner,port);
        return;
    }

    if(ContainsBytes(buf,"rmserver") || ContainsBytes(buf,"realserver")){
        regisger_service("realserver",banner,port);
        return;
    }
    if ((ContainsBytes(line,"ftp")||
        ContainsBytes(line,"winsock")||
        ContainsBytes(line,"axis network camera")||
        ContainsBytes(line,"netpresenz")||
        ContainsBytes(line,"serv-u")||
        ContainsBytes(line,"service ready for new user"))&&
        !HasPrefixString(line,"220")){
        regisger_service("ftp",banner,port);
        return;
    }
    if (HasPrefixString(line,"220-")){
        regisger_service("ftp",banner,port);
        return;
    }
    if (ContainsBytes(line,"220") && ContainsBytes(line,"whois+")){
        regisger_service("whois++",banner,port);
        return;
    }
    if (ContainsBytes(line,"520 command could not be executed")){
        regisger_service("mon",banner,port);
        return;
    }
    
    if (ContainsBytes(line,"ssh-")){
        regisger_service("ssh",banner,port);
        return;
    }
    if (HasPrefixString(line,"+ok") ||
        (line[0]=='+' && ContainsBytes(line,"pop"))){
        mark_pop_server(origline);
        return;
    }
    if (ContainsBytes(line,"imap4") && HasPrefixString(line,"* ok")){
        regisger_service("imap",banner,port);
        return;
    }
    if (ContainsBytes(line,"*ok iplanet messaging multiplexor")){
        regisger_service("imap",banner,port);
        return;
    }

    if (ContainsBytes(line,"*ok communigate pro imap server")){
        regisger_service("imap",banner,port);
        return;
    }

    if (ContainsBytes(line,"* ok courier-imap")){
        regisger_service("imap",banner,port);
        return;
    }

    if (HasPrefixString(line,"giop")){
        regisger_service("giop",banner,port);
        return;
    }

    if (ContainsBytes(line,"microsoft routing server")){
        regisger_service("exchg-routing",banner,port);
        return;
    }
    if (ContainsBytes(line,"gap service ready")){
        regisger_service("iPlanetENS",banner,port);
        return;
    }
    if (ContainsBytes(line,"-service not available")){
        regisger_service("tcpmux",banner,port);
        return;
    }
    if (len(line)> 2 && line[0]== 0x7f && line[1]==0x7f && HasPrefixString(line[2:],"ica")){
        regisger_service("citrix",banner,port);
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
        regisger_service("nntp",banner,port);
        return;
    }
    if (ContainsBytes(buf,"networking/linuxconf")||
        ContainsBytes(buf,"networking/misc/linuxconf")||
        ContainsBytes(buf,"server: linuxconf")){
        regisger_service("linuxconf",banner,port);
        return;
    }
    if (HasPrefixString( buf,"gnudoit:")){
        regisger_service("gnuserv",banner,port);
        return;
    }
    if ((buf[0]==0 && ContainsBytes(buf,"error.host\t1")) ||
        (buf[0]==3 && ContainsBytes(buf,"That item is not currently available"))){
        regisger_service("gopher",banner,port);
        return;
    }
    if(ContainsBytes(buf,"www-authenticate: basic realm=\"swat\"")){
        regisger_service("swat",banner,port);
        return;
    }
    if(ContainsBytes(buf,"vqserver") &&
        ContainsBytes(buf,"www-authenticate: basic realm=/")){
        regisger_service("vqServer-admin",banner,port);
        return;
    }
    if(ContainsBytes(buf,"1invalid request")){
        regisger_service("mldonkey",banner,port);
        return;
    }
    if (ContainsBytes(buf,"get: command not found")){
        regisger_service("wild_shell",banner,port);
        return;
    }

    if (ContainsBytes(buf,"microsoft windows")&&
        ContainsBytes(buf,"c:\\")&&
        ContainsBytes(buf,"(c) copyright 1985-")&&
        ContainsBytes(buf,"microsoft corp.")){
        regisger_service("wild_shell",banner,port);
        return;
    }

    if (ContainsBytes(buf,"netbus")){
        regisger_service("netbus",banner,port);
        return;
    }

    if (ContainsBytes(buf,"netbus")){
        regisger_service("netbus",banner,port);
        return;
    }

    if (ContainsBytes(line,"0 , 0 : error : unknown-error")||
        ContainsBytes(line,"0, 0: error: unknown-error")||
        ContainsBytes(line,"get : error : unknown-error")||
        ContainsBytes(line,"0 , 0 : error : invalid-port")){
        regisger_service("auth",banner,port);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"proxy")){
        regisger_service("http_proxy",banner,port);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"via: ")){
        regisger_service("http_proxy",banner,port);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"proxy-connection: ")){
        regisger_service("http_proxy",banner,port);
        return;
    }
    if (HasPrefixString(line,"http/1.")&&
        ContainsBytes(line,"cache")&&
        ContainsBytes(line,"bad request")){
        regisger_service("http_proxy",banner,port);
        return;
    }
    if (HasPrefixString(origline,"RFB 00")&&
        ContainsBytes(line,".00")){
        regisger_service("vnc",banner,port);
        return;
    }
    if (HasPrefixString(line,"ncacn_http/1.")){
        if(detect_port == 593){
            regisger_service("http-rpc-epmap",banner,port);
        }else{
            regisger_service("ncacn_http");
        }
        return;
    }
    #if (len(line)>=14 && len(line)<=18 && HasPrefixString(_http_req,origline)){
    #    regisger_service("echo",banner,port);
    #    return;
    #}
    if (ContainsBytes(banner,"!\"#$%&'()*+,-./")&&
        ContainsBytes(banner,"ABCDEFGHIJ")&&
        ContainsBytes(banner,"abcdefghij"&&
        ContainsBytes(banner,"0123456789"))){
        regisger_service("chargen",banner,port);
        return;
    }
    if (ContainsBytes(line,"vtun server")){
        regisger_service("vtun",banner,port);
        return;
    }
    if (line=="login: password: "){
        regisger_service("uucp",banner,port);
        return;
    }
    if (line=="bad request" ||
        ContainsBytes(line,"invalid protocol request (71): gget / http/1.0")||
        HasPrefixString(line,"lpd:")||
        ContainsBytes(line,"lpsched")||
        ContainsBytes(line,"malformed from address")||
        ContainsBytes(line,"no connect permissions")){
        regisger_service("ldp",banner,port);
        return;
    }
    if(ContainsBytes(line,"%%lyskom unsupported protocol")){
        regisger_service("lyskom",banner,port);
        return;
    }
    if(ContainsBytes(line,"598:get:command not recognized")){
        regisger_service("ph",banner,port);
        return;
    }
    if(ContainsBytes(origline,"BitTorrent prot")){
        regisger_service("BitTorrent",banner,port);
        return;
    }
    if(ContainsBytes(origline,"BitTorrent prot")){
        regisger_service("BitTorrent",banner,port);
        return;
    }
    if (len(banner)>=4 && banner[0] =='A' && banner[1] == 1 && banner[2]== 2 && banner[3] ==0){
        regisger_service("smux",banner,port);
        return;
    }
    if (HasPrefixString(buf,"0 succeeded\n")){
        regisger_service("LISa",banner,port);
        return;
    }
    if (HasPrefixString(buf,"0 succeeded\n")){
        regisger_service("LISa",banner,port);
        return;
    }
    if (len(banner) == 3 && banner[2]=='\n'){
        regisger_service("msdtc",banner,port);
        return;
    }
    if (HasPrefixString(line,"220") &&
        ContainsBytes(line,"poppassd")){
        regisger_service("pop3pw",banner,port);
        return;
    }
    if (ContainsBytes(line,"welcome!psybnc@")){
        regisger_service("psybnc",banner,port);
        return;
    }
    if (HasPrefixString(line,"* acap ")){
        regisger_service("acap",banner,port);
        return;
    }
    if (ContainsBytes(origline,"Sorry, you (")&&
        ContainsBytes(origline,"are not among the allowed hosts...")){
        regisger_service("nagiosd",banner,port);
        return;
    }
    if (ContainsBytes(line,"[ts].error")||ContainsBytes(line,"[ts].")){
        regisger_service("teamspeak2",banner,port);
        return;
    }
    if (ContainsBytes(origline,"Language received from client:") &&
        ContainsBytes(origline,"Setlocale:")){
        regisger_service("websm",banner,port);
        return;
    }
    if (HasPrefixString(origline,"CNFGAPI")){
        regisger_service("ofa_express",banner,port);
        return;
    }
    if (ContainsBytes(line,"suse meta pppd")){
        regisger_service("smppd",banner,port);
        return;
    }
    if (HasPrefixString(origline,"ERR UNKNOWN-COMMAND")){
        regisger_service("upsmon",banner,port);
        return;
    }
    if (HasPrefixString(line,"connected. ")&&ContainsBytes(line,"legends")){
        regisger_service("sub7",banner,port);
        return;
    }
    if (HasPrefixString(line,"spamd/")){
        regisger_service("spamd",banner,port);
        return;
    }
    if (ContainsBytes(line," dictd ")&&HasPrefixString(line,"220")){
        regisger_service("dicts",banner,port);
        return;
    }
    if (ContainsBytes(line," dictd ")&&HasPrefixString(line,"220")){
        regisger_service("dicts",banner,port);
        return;
    }
    if (HasPrefixString(line,"220 ")&&ContainsBytes(line,"vmware authentication daemon")){
        regisger_service("vmware_auth",banner,port);
        return;
    }
    if (HasPrefixString(line,"220 ")&&ContainsBytes(line,"interscan version")){
        regisger_service("interscan_viruswall",banner,port);
        return;
    }
    if (len(banner) >1 && (banner[0] == '~')
        && (banner[len(banner) - 1] == '~')
        && ContainsBytes(banner,"}")){
        regisger_service("pppd",banner,port);
        return;
    }
    if (ContainsBytes(banner,"Hello, this is zebra ")){
        regisger_service("zebra",banner,port);
        return;
    }
    if (ContainsBytes(banner,"Hello, this is zebra ")){
        regisger_service("zebra",banner,port);
        return;
    }
    if (ContainsBytes(line,"ircxpro ")){
        regisger_service("ircxpro_admin",banner,port);
        return;
    }
    if (HasPrefixString(origline,"version report")){
        regisger_service("gnocatan",banner,port);
        return;
    }
    if (HasPrefixString(origline,"RTSP/1.0")&&ContainsBytes(origline,"QTSS/")){
        regisger_service("quicktime-streaming-server",banner,port);
        return;
    }
    if (len(origline) >=2 && origline[0]==0x30 && origline[1]=0x11 && origline[2]==0){
        regisger_service("dameware",banner,port);
        return;
    }
    if (ContainsBytes(line,"stonegate firewall")){
        regisger_service("SG_ClientAuth",banner,port);
        return;
    }
    if (HasPrefixString(line,"pbmasterd")){
        regisger_service("power-broker-master",banner,port);
        return;
    }
    if (HasPrefixString(line,"pblocald")){
        regisger_service("power-broker-locald",banner,port);
        return;
    }
    if (HasPrefixString(line,"<stream:error>invalid xml</stream:error>")){
        regisger_service("jabber",banner,port);
        return;
    }
    if (HasPrefixString(line,"/c -2 get ctgetoptions")){
        regisger_service("avotus_mm",banner,port);
        return;
    }
    if (HasPrefixString(line,"error:wrong password")){
        regisger_service("pNSClient",banner,port);
        return;
    }
    if (HasPrefixString(line,"1000      2")){
        regisger_service("VeritasNetBackup",banner,port);
        return;
    }
    if (ContainsBytes(line,"the file name you specified is invalid")
        &&ContainsBytes(line,"listserv")){
        regisger_service("listserv",banner,port);
        return;
    }
    if (HasPrefixString(line,"control password:")){
        regisger_service("FsSniffer",banner,port);
        return;
    }
    if (HasPrefixString(line,"remotenc control password:")){
        regisger_service("RemoteNC",banner,port);
        return;
    }
    if (ContainsBytes(banner,"finger: GET: no such user")||
        ContainsBytes(banner,"finger: /: no such user")||
        ContainsBytes(banner,"finger: HTTP/1.0: no such user")){
        regisger_service("finger",banner,port);
        return;
    }
    if (len(banner)>=4 && banner[0]==5 && banner[1] <=8 && banner[2] == 0 && banner[3]<=4){
        regisger_service("socks5",banner,port);
        return;
    }
    if (len(banner)>=2 && banner[0] == 0 && banner[1]>=90 && banner[1]<=93){
        regisger_service("socks4",banner,port);
        return;
    }
    if (ContainsBytes(buf,"it looks like you are trying to access mongodb over http on the native driver port.")){
        regisger_service("mongodb",banner,port);
        return;
    }

    if (len(banner)>=4 && (banner[0]>='0' && banner[0]<='9')
        &&(banner[1]>='0' && banner[1]<='9')
        &&(banner[2]>='0' && banner[2]<='9')
        &&(banner[3]==0 || banner[3]==0x20 || banner[3]=='-')){
        regisger_service("three_digits",banner,port);
        return;
    }

    if(!foundWWW && port != 139 && port != 445 && port != 135){
        regisger_service("unknown",banner,port);
    }
}



func detect_tcp_service(host,port){
    var host_field = host;
    if(port != 80 || port != 443){
        host_field += (":"+port);
    }
    var req = "GET / HTTP/1.0\r\nHost: "+host_field+"\r\n\r\n";
    var banner = grab_banner(host,port,false,nil);
    if(!banner){
        banner = grab_banner(host,port,false,req);
    }
    if(banner){
        #HexDumpBytes(banner);
        if(banner == req){
            regisger_service("echo",banner,port);
        }
        detect_banner(banner,port);
        return;
    }
    banner = grab_banner(host,port,true,nil);
    if(!banner){
        banner = grab_banner(host,port,true,req);
    }
    if(banner){
        #HexDumpBytes(banner);
        if(banner == req){
            regisger_service("echo",banner,port);
        }
        detect_banner(banner,port);
        return;
    }
}

#detect_tcp_service("192.168.0.106",5900);
#detect_tcp_service("192.168.0.106",80);
#detect_tcp_service("192.168.0.106",22);
#detect_tcp_service("192.168.0.106",443);