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
    if(len(line)<=0){
        return 1;
    }
    if(HasPrefixString(line,"230")){
        count = 0;
        for {
            if(len(line)>4 && line[3]=='-' && count < 1024){
                line = recv_line(socket,1024);
                count++;
            }else{
                break;
            }
        }
    }
    if(HasPrefixString(line,"331")){
        return 1;
    }
    count = 0;
    for {
        if(len(line)>4 && line[3]=='-' && count < 1024){
            line = recv_line(socket,1024);
            count++;
        }else{
            break;
        }
    }
    return 0;
}

#Entering Passive Mode (172,16,62,72,17,118)
func ftp_get_pasv_port(socket){
    ConnWrite(socket,"PASV\r\n");
    var line = recv_line(socket,512);
    if(len(line)<=0){
        return 0;
    }
    if(!HasPrefixString(line,"227")){
        return 0;
    }
    var pos = IndexString(line,"(");
    if(pos == -1){
        return 0;
    }
    line = line[pos+1:];
    line = line[:len(line)-1];
    var list = SplitString(line,",");
    if(len(list)==6){
        return ToInteger(list[4])<<16+ToInteger(list[5]);
    }
    if(len(list)==5){
        return ToInteger(list[4]);
    }
    return 0;
}