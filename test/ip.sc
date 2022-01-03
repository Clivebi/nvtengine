
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

func set_ipv6_enc(hdr,enc){
    enc = (enc & 0x3F)<<20;
    hdr=WriteUInt32(hdr,0,ReadUInt32(hdr,0)|(enc&0xFFFFFFFF));
    return hdr;
}

func set_ipv6_flow(hdr,flow){
    flow = (flow & 0xFFFFF);
    hdr=WriteUInt32(hdr,0,ReadUInt32(hdr,0)|(flow&0xFFFFFFFF));
    return hdr;
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
    return ToString(src[0])+"."+ToString(src[1])+"."+ToString(src[2])+"."+ToString(src[3]);
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
ip_ttl=64,ip_p=0,ip_sum=0,ip_src="",ip_dst=""){
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

func forge_ip_v6_packet(data="",ip6_v=6,ip6_tc,ip6_fl,ip6_p,ip6_hlim,ip6_src,ip6_dst){
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
            return ReadUInt16(4,false);
        }
        case "ip_hl":{
            return get_ipv4_hl(ip);
        }
        case "ip_tos":{
            return get_ipv4_tos(ip);
        }
        case "ip_len":{
            return ReadUInt16(ip,2,false);
        }
        case "ip_off":{
            return ReadUInt16(hdr,6,false);
        }
        case "ip_ttl":{
            return get_ipv4_ttl(hdr);
        }
        case "ip_p":{
            return get_ipv4_protocol(hdr);
        }
        case "ip_sum":{
            return get_ipv4_checksum(hdr);
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

func dump_ip_packet(packet){
     Println(HexDumpBytes(packet));
}

func dump_ip_v6_packet(packet){
    Println(HexDumpBytes(packet));
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
    Println(HexDumpBytes(packet));
}

func dump_tcp_v6_packet(packet){
    Println(HexDumpBytes(packet));
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

func forge_udp_v6_packet(ip, data, uh_dport=0, uh_sport=0, uh_sum,uh_ulen, update_ip_len){
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
    if(element != "data"){
        var offset = ipsz+8;
        var datasz = ReadUInt16(hdr,4);
        if(datasz +ipsz > len(udp)){
            datasz = len(udp)-ipsz;
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
    if(!udp){
        return nil;
    }
    if(udp[0]>>4 != 6){
        return nil;
    }
    var ip_len = get_ipv6_plen(tcp);
    if(ip_len > len(tcp)){
        return nil;
    }
    var hdr = udp[40:];
    if(element != "data"){
        var offset = 48;
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

func dump_udp_packet(packet){
    Println(HexDumpBytes(packet));
}

func dump_udp_v6_packet(packet){
    Println(HexDumpBytes(packet));
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
    icmp = WriteUInt16(icmp,2,sum);
    ip = set_ipv4_length(ip,len(sum));
    ip = update_ipv4_checksum(ip);
    return append(bytes(ip),icmp);
}

func forge_icmp_v6_packet(ip6,data="",icmp_type,icmp_code,icmp_id,icmp_seq,
                        reachable_time,retransmit_timer,flags,target,icmp_cksum,update_ip_len=true){
    
}

func get_icmp_element(){

}

func get_icmp_v6_element(){

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

func send_packet(){

}

func send_v6packet(){

}

func pcap_next(){

}

func send_capture(){
    
}

func test_address_convert(){
    #fe80::872:545c:cf3b:e0f5
    #ff02::16
    #1050:0:0:0:5:600:300c:326b
    Println(HexEncode(ipv6_string_to_address("fe80::872:545c:cf3b:e0f5")));
    Println(HexEncode(ipv6_string_to_address("ff02::16")));
    Println(HexEncode(ipv6_string_to_address("1050:0:0:0:5:600:300c:326b")));
}


func test_udp(){
    var _dns = HexDecodeString("df3301000001000000000000066173696d6f7606766f7274657804646174610e747261666669636d616e61676572036e65740000010001");
    
    #ip_hl,ip_tos,ip_len,ip_id,ip_off_flags,ip_off,ip_ttl,ip_p,ip_src,ip_dst
    var _ip_hdr = build_ipv4_header(5,0,00,0xd83e,0,0,64,17,ipv4_string_to_address("192.168.4.149"),ipv4_string_to_address("114.114.114.114"));
    var _upd_hdr = build_udp(_ip_hdr,60075,53,_dns);
    _ip_hdr = set_ipv4_length(_ip_hdr,len(_upd_hdr)+get_ipv4_hl(_ip_hdr)*4);
    var _total = append(_ip_hdr,_upd_hdr);
    _total = update_ipv4_checksum(_total);
    var raw = HexDecodeString("45000053d83e00004011f839c0a8049572727272eaab0035003fe4dadf3301000001000000000000066173696d6f7606766f7274657804646174610e747261666669636d616e61676572036e65740000010001");
    if (raw != _total){
        Println(HexDumpBytes(raw));
        Println(HexDumpBytes(_total));
    }
    #func forge_ip_packet(data="",ip_hl=5,ip_v=4,ip_tos=0,ip_id=0,ip_off=0,
    #ip_ttl=64,ip_p=0,ip_sum=0,ip_src="",ip_dst="")
    #func forge_udp_packet(ip, data, uh_dport=0, uh_sport=0, uh_sum,uh_ulen, update_ip_len)

    ip = forge_ip_packet("",5,4,0,0xd83e,0,64,17,0,"192.168.4.149","114.114.114.114");
    udp = forge_udp_packet(ip,_dns,53,60075,0,0,0);
    if (raw != udp){
        Println("**************************");
        Println(HexDumpBytes(raw));
        Println(HexDumpBytes(udp));
    }
}


func test_tcp_option(){
    #ip_hl,ip_tos,ip_len,ip_id,ip_off_flags,ip_off,ip_ttl,ip_p,ip_src,ip_dst
    var _ip_hdr =  build_ipv4_header(5,00,00,0,2,0,64,6,ipv4_string_to_address("192.168.4.149"),ipv4_string_to_address("23.202.34.33"));
    var _tcp = build_tcp_header(60113,443,3824330984,958269534,0x010,2048,0);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    var tp = MakeBytes(8);
    WriteUInt32(tp,0,1039490342);
    WriteUInt32(tp,4,2774456090);
    _tcp = add_tcp_option(_tcp,0x8,10,tp);
    set_ipv4_length(_ip_hdr,len(_tcp)+get_ipv4_hl(_ip_hdr)*4);
    update_ipv4_checksum(_ip_hdr);
    update_tcp_checksum(_ip_hdr,_tcp,nil);
    var _total = append(_ip_hdr,_tcp);
    var raw = HexDecodeString("450000340000400040063b9cc0a8049517ca2221ead101bbe3f2a8e8391e085e801008009d1b00000101080a3df55d26a55ed71a");
    if(raw != _total ){
       Println("error ....");
       Println(HexDumpBytes(_total));
       Println(HexDumpBytes(raw));
   }
   #func forge_ip_packet(data="",ip_hl=5,ip_v=4,ip_tos=0,ip_id=0,ip_off=0,
   #ip_ttl=64,ip_p=0,ip_sum=0,ip_src="",ip_dst="")

   #func build_tcp_header(src_port,dst_port,seq,ack,flags,win,urp)
   #func forge_tcp_packet(ip,data=nil,th_ack=0,th_dport,th_flags,
   #                   th_off,th_seq=0,th_sport,th_sum,th_urp,th_win,th_x2,update_ip_len)

   var ip = forge_ip_packet("",5,4,0,0,2<<13,64,6,0,"192.168.4.149","23.202.34.33");
   var tcp = forge_tcp_packet(ip:ip,th_sport:60113,th_dport:443,th_ack:958269534,th_seq:3824330984,th_flags:0x010,th_win:2048,th_urp:0);
   tcp = add_tcp_option(tcp,0x1,nil,nil);
   tcp = add_tcp_option(tcp,0x1,nil,nil);
   tcp = add_tcp_option(tcp,0x8,10,tp);
}

func test_tcp_payload(){
    var data = "160303007a020000760303a303b463e563d7ee6a1256444243241cd818d5abb96d9a9dfb6b8989a6162343206607a732095aa015b09a564068e4de938586f09f0bdc4582d96aefa2bef7963d130200002e002b0002030400330024001d002051ce8edc33e811c98ed6113c91ef4380843ba17d2d49c1142a221f2b37f4f16d140303000101170303003403687bbbdbbfcbea553ce2bb7cea3c430a9b65750ba7fe9e7f42369e0f47cd679f2740f4953dfa6b4b325dfccb325c659ccb9c351703030db2f8f7746156911628a41e29745248409c7dda101e0af723a6a250584591f270fc1f872d9aab58a26292d9521ac51c5140adb12aa9ab0f344a4af312aa09380c1002a63c208af60730f63d0e48413edd3cefe0c8f7c980984161b1ab61b837352c1f1bd184f3a1e1e5abe21a58245b4a55a728f60fe69814460cbb67268024d8b560f53d4928b2161b878db8aa743064f4bb281a0093e39bf26bdfabb54076ce81a4162336380f014d4b5989f9d5c0f0b6b4f7715c3b23c5c6f7fea7dbd9855dd5e8c9367894c4aaed8b1a40955d17bb930da57d985976e608d72ba7d1f5cf96e435963a86d8db922c8864cb96c9aeeb1ff50b1e6475d54dcab5bf9b4a2973dc897b66417e8444b54c91c7148ec4822875142aebfdc5e8040d8b8589f5cfab0ce6b85000cc7a3f255c7a4983fcbfd970aff89a3c1acacd88c6047f45023187d91c9405c0bf8634d17ad0cc93b832eef3d19e968bc4192e0ab43910d8282531f3a3f3cea742241b7a54359e8e5b864d42c2a40c40f0abf2a4a878aadbf680a2ad5467433064508903f0b632fc1e268b9713d62ea6d8de951ded58b902aecb18c63a29c8fdd91f1b83018ede2bdca38d0cd47282f06d52703ec8946415c0d5a2617ca043ac5cd9b55c6004e2ca35ae05e3fa67ee274d10ce2f946ce7196081473023c1934e356ba3cf9ce13f36b80768c63b6bd1d8b47e2ee928b1b3001a2786da40f9cb4772b3edca6d5fdf397c18d52580dea537fc430a00a66bec06dc45608af03a1b5f2d7c4a552da4bedf1ec838cb0ac6e3f622b9d35d78a7eb9541629d500c44e37fa8c0904af057d27a91788826ee7108b05af4615685f8fa272d271e4a014565ef45be887da6247a119f940f6024916cba06a45e2e77f4de36462410d338122985c2ec0bb547ce273162bab953b767e085bcea257cd71c946d81a572ebc15b0cf68867a2bdadd0b50194a5ee2acdf3341e5c86daaff7f3f83f968e29501c2c2aee0eb02c589322b2d354614cf14efdaa5ba968bae34203b2a43c4f2e0a7f7a19980a93c417906f30f344ce704398912dd4b51b4d72641233a6fc626fb4bb8a33466db4ca8c39c5553345dbf53bb6683e86e56cdb42385f1448968bc3b2aa7e44980ab00772ac20c475f8ac7a21f115bcf90ec2b7c1cb84b4df1195d21fe646c84a477ff4038deb3f5bb8783a226f0d73ef38a3f707e4748a7407bdcefc99c1b9fee24b9f69b38f01dc24bea2803a28d59f270bf07319def82971a44094a746b6fdf559ddcf2702bafea59176d1bc88955f606714d42b1e5c5a96aaa2e9beb38041cf327e018fbc701656fe92e2d5341e6d295fdd491155cc22cab4131e9659c7a09ae0e2a0db2a1846fff1f9482ae2e17e1818c77d7bb73543c3d9963b31f8fce638c9febb7dccbc2dd844306277224dbc09932692648b94b2eeeb1b0557b3e68b3ee9b41480f05c70fcb2f3a49233a8c0d8d49bbb8b8f0a2b7feb4dd6383ac6d0628ac3781aa8aafa4c9b0613c54e5c586b4e183d59996a6fbf8dad773ef4f9d7fc5db0b13b41b3f28623734fb0527a89dff1ac1d09a7b770a874e2a6f2dca03f0204de0110d2882d6d882da5d7e040f8ee8e3e2e0cddd2571f9176faa8638481cc94e3f96af4cdee7c711aa8b022ff94f14d43f2148f0a230ba9d993b1908b76b621028cac9ccd527c5bcb580bbc2d0d8835d5398e61bf2e16a1624a0f71332406276c83811dddb7e70263118e1f1d76ea579c1c08bb656a1ab1ba7063059219b3b9";
    data = HexDecodeString(data);
    var _ip_hdr =  build_ipv4_header(5,2,00,0xC44f,2,0,47,6,ipv4_string_to_address("23.202.34.33"),ipv4_string_to_address("192.168.4.149"));
    var _tcp = build_tcp_header(443,60113,958265658,3824330984,0x010,235,0);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    var tp = MakeBytes(8);
    WriteUInt32(tp,0,2774456090);
    WriteUInt32(tp,4,1039490327);
    _tcp = add_tcp_option(_tcp,0x8,10,tp);
    set_ipv4_length(_ip_hdr,len(_tcp)+len(data)+get_ipv4_hl(_ip_hdr)*4);
    update_ipv4_checksum(_ip_hdr);
    update_tcp_checksum(_ip_hdr,_tcp,data);
    var _total = append(_ip_hdr,_tcp);
    if(HexEncode(_total)!= "450205dcc44f40002f0682a217ca2221c0a8049501bbead1391df93ae3f2a8e8801000eb7afa00000101080aa55ed71a3df55d17"){
        Println("error ....");
        Println(HexDumpBytes(_total)); 
        var raw = HexDecodeString("450205dcc44f40002f0682a217ca2221c0a8049501bbead1391df93ae3f2a8e8801000eb7afa00000101080aa55ed71a3df55d17");
        Println(HexDumpBytes(raw));
    }
}

func test_ipv6_udp(){
    var payload = HexDecodeString("000084000000000400000001096d696e69706f642d4c056c6f63616c00001c8001000000780010fe8000000000000014cddc4efc96c36ec00c00018001000000780004c0a80499c00c001c8001000000780010fd7727a62bc94b390000fdd5e521db8dc00c001c8001000000780010fd7727a62bc94b39002e0fc3df673729c00c002f8001000000780008c00c000440000008");
    var _ip_hdr = build_ipv6_header(0,0x600,17,255,HexDecodeString("fe8000000000000014cddc4efc96c36e"),
    HexDecodeString("ff0200000000000000000000000000fb"),0);
    var raw = HexDecodeString("60000600009b11fffe8000000000000014cddc4efc96c36eff0200000000000000000000000000fb14e914e9009bb8f2000084000000000400000001096d696e69706f642d4c056c6f63616c00001c8001000000780010fe8000000000000014cddc4efc96c36ec00c00018001000000780004c0a80499c00c001c8001000000780010fd7727a62bc94b390000fdd5e521db8dc00c001c8001000000780010fd7727a62bc94b39002e0fc3df673729c00c002f8001000000780008c00c000440000008");
    var _udp = build_udp(_ip_hdr,5353,5353,payload);
    _ip_hdr = set_ipv6_plen(_ip_hdr,len(_udp));
    var _total = append(_ip_hdr,_udp);
    if (raw != _total){
        Println("error...");
        Println(HexDumpBytes(_total));
        Println(HexDumpBytes(raw));
    }
}

func test_ipv6_tcp(){
    var _ip_hdr = build_ipv6_header(0,0xb0f00,6,64,HexDecodeString("fe80000000000000003df870a1ee7fb1"),
    HexDecodeString("fe800000000000000c1a7e7c586e0e27"),0);
    var _tcp = build_tcp_header(60215,62078,1709947670,305574406,0x010,2052,0);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    var tp = MakeBytes(8);
    WriteUInt32(tp,0,2582551924);
    WriteUInt32(tp,4,992426282);
    _tcp = add_tcp_option(_tcp,0x8,10,tp);
    _ip_hdr = set_ipv6_plen(_ip_hdr,len(_tcp));
    _tcp = update_tcp_checksum(_ip_hdr,_tcp,nil);
   var _total = append(_ip_hdr,_tcp);
   var raw = HexDecodeString("600b0f0000200640fe80000000000000003df870a1ee7fb1fe800000000000000c1a7e7c586e0e27eb37f27e65ebbb161236b20680100804f79300000101080a99ee9d743b27392a");
   if (raw != _total){
        Println("error...");
        Println(HexDumpBytes(_total));
        Println(HexDumpBytes(raw));
    }
}

func test_ipv6_tcp_with_payload(){
    var data = HexDecodeString("3c3f786d6c2076657273696f6e3d22312e302220656e636f64696e673d225554462d38223f3e0a3c21444f435459504520706c697374205055424c494320222d2f2f4170706c652f2f44544420504c49535420312e302f2f454e222022687474703a2f2f7777772e6170706c652e636f6d2f445444732f50726f70657274794c6973742d312e302e647464223e0a3c706c6973742076657273696f6e3d22312e30223e0a3c646963743e0a093c6b65793e4c6162656c3c2f6b65793e0a093c737472696e673e7573626d7578643c2f737472696e673e0a093c6b65793e50726f746f636f6c56657273696f6e3c2f6b65793e0a093c737472696e673e323c2f737472696e673e0a093c6b65793e526571756573743c2f6b65793e0a093c737472696e673e5175657279547970653c2f737472696e673e0a3c2f646963743e0a3c2f706c6973743e0a");
    var _ip_hdr = build_ipv6_header(0,0xb0f00,6,64,HexDecodeString("fe80000000000000003df870a1ee7fb1"),
    HexDecodeString("fe800000000000000c1a7e7c586e0e27"),0);
    var _tcp = build_tcp_header(60215,62078,1709947674,305574406,0x018,2052,0);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    _tcp = add_tcp_option(_tcp,0x1,nil,nil);
    var tp = MakeBytes(8);
    WriteUInt32(tp,0,2582551929);
    WriteUInt32(tp,4,992426288);
    _tcp = add_tcp_option(_tcp,0x8,10,tp);
    _ip_hdr = set_ipv6_plen(_ip_hdr,len(_tcp)+len(data));
    _tcp = update_tcp_checksum(_ip_hdr,_tcp,data);
   var _total = append(_ip_hdr,_tcp);
   var raw = HexDecodeString("600b0f0001680640fe80000000000000003df870a1ee7fb1fe800000000000000c1a7e7c586e0e27eb37f27e65ebbb1a1236b20680180804f55a00000101080a99ee9d793b273930");
   if (raw != _total){
        Println("error...");
        Println(HexDumpBytes(_total));
        Println(HexDumpBytes(raw));
    }
}

test_address_convert();
test_udp();
test_tcp_option();
test_tcp_payload();
test_ipv6_udp();
test_ipv6_tcp();
test_ipv6_tcp_with_payload();
