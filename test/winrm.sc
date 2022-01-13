
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
        Println(HexDumpBytes(result["StdErr"]));
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

#host,port,login,password,ishttps,inscure,ca(base64 encode),cert(base64 encode),certkey,timeout( second).useNTLM
var winrm = CreateWinRM("192.168.4.180",5985,"Lewis","Lewis123",false,true,"","","",15,true);
if(winrm == nil){
    Println("CreateWinRM failed...");
    exit(0);
}
Println(RegGetValue(winrm,"HKLM\\Software\\Microsoft\\Driver Signing","Policy"));
Println(wmi_reg_get_bin_val(winrm,"Software\\Microsoft\\Driver Signing","Policy"));
Println(wmi_reg_get_bin_val(winrm,"System\\CurrentControlSet\\Control\\Lsa","fullprivilegeauditing"));
Println(wmi_reg_enum_key(winrm,"Software\\7-Zip"));
Println(wmi_reg_enum_value(winrm,"Software\\7-Zip"));
Println(wmi_reg_get_sz(winrm,"Software\\7-Zip","Path"));
Println(wmi_reg_get_dword_val(winrm,"Software\\7-Zip","Test1"));
Println(wmi_reg_get_ex_string_val(winrm,"Software\\7-Zip","Test5"));
Println(wmi_reg_get_mul_string_val(winrm,"Software\\7-Zip","Test3"));
Println(wmi_reg_get_bin_val(winrm,"Software\\7-Zip","Test4"));
Println(RegEnumKey(winrm,"HKLM\\Software\\7-Zip"));
Println(RegEnumValue(winrm,"HKLM\\Software\\7-Zip"));
Println(RegGetValue(winrm,"HKLM\\Software\\7-Zip","Path"));
#Println(RegAddKey(winrm,"HKLM\\Software\\7-Zip\\Scaned\""));
#Println(RegAddValue(winrm,"HKLM\\Software\\7-Zip\\Scaned\"",{"name":"Test\"","type":"REG_SZ","value":"TestValue"}));
Println(wmi_query(winrm,"SELECT ProcessId,Name,ExecutablePath FROM Win32_Process"));
Println(wmi_query(winrm,"SELECT Name, TotalPhysicalMemory FROM Win32_Computersystem"));
Println(wmi_query(winrm,"SELECT * FROM Win32_Processor"));
Println(wmi_query(winrm,"SELECT DeviceID, Name, NumberOfCores FROM Win32_Processor"));
Println(wmi_query(winrm,"SELECT DeviceID, Manufacturer, Name FROM Win32_PNPEntity WHERE DeviceID LIKE '%PCI\\\\VEN_%' "));
Println(wmi_query(winrm,"SELECT Description, Index, IPAddress, MACAddress FROM Win32_NetworkAdapterConfiguration"));