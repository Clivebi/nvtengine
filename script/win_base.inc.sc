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

func _remove_some_properties(item){
    if(item){
        delete(item,"SystemProperties");
        delete(item,"Qualifiers");
        delete(item,"Scope");
        delete(item,"Properties");
    }
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
                _remove_some_properties(item);
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
        _remove_some_properties(item);
        ret = append(ret,item);
    }
    return ret;
}

func ExecuteRegCommand(handle,cmd){
    var result = WinRMCommand(handle,cmd,"",false);
    if (result ==nil){
        return nil;
    }
    if(result.ExitCode != 0 || len(result.StdErr)>0){
        if(ContainsString(result.StdErr,"The system was unable to find the specified registry key or value")){
            return nil;
        }
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

func _reg_binay_convert(str){
    var res = 0;
    for(var i =0;i<len(str);i++){
        if(str[i]=='1'){
            res |=(1<<i);
        }
    }
    return res;
}

func _reg_one_byte(iv){
    if(iv==0){
        return "0";
    }
    return byte(iv);
}

func RegDecodeBinaryValue(str){
    var result = "",temp = "";
    for{
        if(len(str)<=8){
            result += _reg_one_byte(_reg_binay_convert(str));
            return result;
        }
        temp = str[:8];
        str =str[8:];
        result += _reg_one_byte(_reg_binay_convert(temp));
    }
}

func GetOperatingSystemInformation(handle){
    var infos = WMIQuery(handle,"SELECT * FROM Win32_OperatingSystem");
    if(!infos){
        return nil;
    }
    if(typeof(infos)=="array" && len(infos)){
        return infos[0];
    }
    return nil;
}

func GetProcessInformation(handle){
     return WMIQuery(handle,"SELECT * FROM Win32_Process");
}

func GetServicesInformation(handle){
      return WMIQuery(handle,"SELECT * FROM Win32_Service");
}

func _warp_single_qute_string(str){
    str = ReplaceAllString(str,"\\","\\\\");
    return "'"+str+"'";
}

func GetFileInformation(handle,filepath){
    var wql = "SELECT * FROM CIM_DataFile WHERE Name="+_warp_single_qute_string(filepath);
    return WMIQuery(handle,wql);
}

#File directory API
func GetFileVersion(handle,filepath){
    info = GetFileInformation(handle,filepath);
    if(!info || len(info) == 0){
        return nil;
    }
    return info[0]["Version"];
}

func GetFileFileSize (handle,filepath){
    info = GetFileInformation(handle,filepath);
    if(!info || len(info) == 0){
        return nil;
    }
    return info[0]["FileSize"];
}

func GetFileFileType (handle,filepath){
    info = GetFileInformation(handle,filepath);
    if(!info || len(info) == 0){
        return nil;
    }
    return info[0]["FileType"];
}

func EnumChildDirectoryFull(handle,dir){
    if(!HasSuffixBytes(dir,"\\")){
        dir += "\\";
    }
    if(dir[1]=':'){
        var wql = "SELECT * FROM Win32_Directory WHERE ";
        wql += "Drive=";
        wql += _warp_single_qute_string(dir[:2]);
        wql += " AND Path=";
        wql += _warp_single_qute_string(dir[2:]);
    }else{
        var wql = "SELECT * FROM Win32_Directory WHERE Path="+_warp_single_qute_string(dir);
    }
    return WMIQuery(handle,wql);
}

func EnumChildDirectoryWithLightInformation(handle,dir){
    if(!HasSuffixBytes(dir,"\\")){
        dir += "\\";
    }
    if(dir[1]=':'){
        var wql = "SELECT Name,FileName,Extension,FileSize,FileType FROM Win32_Directory WHERE ";
        wql += "Drive=";
        wql += _warp_single_qute_string(dir[:2]);
        wql += " AND Path=";
        wql += _warp_single_qute_string(dir[2:]);
    }else{
        var wql = "SELECT Name,FileName,Extension,FileSize,FileType FROM Win32_Directory WHERE Path="+_warp_single_qute_string(dir);
    }
    return WMIQuery(handle,wql);
}

func EnumChildFileFull(handle,dir){
    if(!HasSuffixBytes(dir,"\\")){
        dir += "\\";
    }
    if(dir[1]=':'){
        var wql = "SELECT * FROM CIM_DataFile WHERE ";
        wql += "Drive=";
        wql += _warp_single_qute_string(dir[:2]);
        wql += " AND Path=";
        wql += _warp_single_qute_string(dir[2:]);
    }else{
        var wql = "SELECT * FROM CIM_DataFile WHERE Path="+_warp_single_qute_string(dir);
    }
    return WMIQuery(handle,wql);
}

func EnumChildFileWithLightInformation(handle,dir){
    if(!HasSuffixBytes(dir,"\\")){
        dir += "\\";
    }
    if(dir[1]=':'){
        var wql = "SELECT Name,FileName,Extension,FileSize,FileType FROM CIM_DataFile WHERE ";
        wql += "Drive=";
        wql += _warp_single_qute_string(dir[:2]);
        wql += " AND Path=";
        wql += _warp_single_qute_string(dir[2:]);
    }else{
        var wql = "SELECT Name,FileName,Extension,FileSize,FileType FROM CIM_DataFile WHERE Path="+_warp_single_qute_string(dir);
    }
    return WMIQuery(handle,wql);
}

func GetDirectoryInformation(handle,dir){
    var wql = "SELECT * FROM Win32_Directory WHERE Name="+_warp_single_qute_string(dir);
    var info = WMIQuery(handle,wql);
    if(info && len(info)){
        return info[0];
    }
    return nil;
}


func GetFileContent(handle,path){
    var cmd = "[System.Convert]::ToBase64String([System.IO.File]::ReadAllBytes("+_warp_single_qute_string(path) + "))";
    var result = WinRMCommand(handle,cmd,"",true);
    if(!result){
        return nil;
    }
    if (result.ExitCode != 0 || len(result.StdErr) > 0){
        Println(result);
        return nil;
    }
    return StdBase64Decode(result.Stdout);
}
