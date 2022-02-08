require("nasl.sc");

var soc = TCPConnect("192.168.1.47",21,15,false,true);
Println(ftp_log_in(soc,"Anonymous"," "));
Println(ftp_get_pasv_port(soc));