func ftp_log_in(socket,user,pass){
    var line = recv_line(socket,1024);
    if(!HasPrefixString(line,"220")){
        return 1;
    }
    var count = 0;
    for {
        if(len(line)>4 && line[3]=='-' && count < 1024){
            line = recv_line(socket,1024);
            count++;
        }else{
            break;
        }
    }
    if(count >=1024 || len(line)<=0){
        return 1;
    }
    var req = "USER "+user+"\r\n";
    var nSize = ConnWrite(socket,req);
    if (nSize <= 0){
        return 1;
    }
    line = recv_line(socket,1024);
}