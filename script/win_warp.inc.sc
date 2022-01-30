require("nasl.sc");
require("win_base.inc.sc");

func GetWinRM(){
    var winrm = get_shared_object("winrm_handle");
    if(!winrm){
        var user = get_kb_item("Secret/WinRM/login");
        var key = get_kb_item("Secret/WinRM/password");
        var port = get_kb_item("Secret/WinRM/transport");
        if(!port){
            port = 5985;
        }
        if (!user || len(user) == 0 || !key || len(key) ==0){
            return nil;
        }
        winrm = CreateWinRM(get_host_ip(),port,user,key,false,true,"","","",60,true);
        if(winrm == nil){
            return nil;
        }
        var wql ="SELECT Caption FROM Win32_OperatingSystem";
        var info = WMIQuery(winrm,wql);
        if(!info || len(info) ==0){
            return nil;
        }
        set_shared_object("winrm_handle",winrm);
    }
    return winrm;
}

func SetWinRMHandle(handle){
    set_shared_object("winrm_handle",handle);
}

func IsWindowsHostAccessable(){
    var winrm = GetWinRM();
    return winrm != nil;
}

func GetWindowsDirectory(){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    var info = GetOperatingSystemInformation(winrm);
    if(info){
        return info.WindowsDirectory;
    }
    return "";
}

func GetSystemDirectory(){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    var info = GetOperatingSystemInformation(winrm);
    if(info){
        return info.SystemDirectory;
    }
    return "";
}

func GetWindowsBuildNumber(){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    var info = GetOperatingSystemInformation(winrm);
    if(info){
        return info.BuildNumber;
    }
    return "";
}

func GetWindowsCaption(){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    var info = GetOperatingSystemInformation(winrm);
    if(info){
        return info.Caption;
    }
    return "";
}

func GetWindowsVersion(){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    var info = GetOperatingSystemInformation(winrm);
    if(info){
        return info.Version;
    }
    return "";
}

func IsFileExist(file){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    return GetFileFileType(winrm,file) != nil;
}

func IsPathExist(path){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    if (GetFileFileType(winrm,path)){
        return true;
    }
    if(GetDirectoryInformation(winrm,path)){
        return true;
    }
    return false;
}

func GetFileBaseName(str){
    var pos = LastIndexString(str,"\\");
    if(pos == -1){
        return str;
    }
    return str[pos+1:];
}

func GetChildDirectoryNameList(path,retfullpath=true){
    var winrm = GetWinRM();
    if(!winrm){
        return nil;
    }
    var list = EnumDirectoryWithLightInformation(winrm,path);
    if(list && len(list)){
        var ret = [];
        for v in list{
            if(retfullpath){
                ret += v.Name;
            }else{
                ret += GetFileBaseName(v.Name);
            }
        }
        return ret;
    }
    return nil;
}

func GetChildFileNameList(path,retfullpath=true){
    var winrm = GetWinRM();
    if(!winrm){
        return "";
    }
    var list = EnumChildFileWithLightInformation(winrm,path);
    if(list && len(list)){
        var ret = [];
        for v in list{
            if(retfullpath){
                ret += v.Name;
            }else{
                ret += GetFileBaseName(v.Name);
            }
        }
        return ret;
    }
    return nil;
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

func wmi_connect(username,password,ns=nil,host=""){
    if(host == "" || host == nil){
        host = get_host_ip();
    }
    var winrm = CreateWinRM(host,5985,username,password,false,true,"","","",get_default_connect_timeout(),true);
    if(winrm == nil){
        return nil;
    }
    return winrm;
}

func wmi_close(wmi_handle){
    if(wmi_handle){
        close(wmi_handle);
    }
}

func wmi_connect_reg(host="",username,password){
    if(host == "" || host == nil){
        host = get_host_ip();
    }
    var winrm = CreateWinRM(host,5985,username,password,false,true,"","","",get_default_connect_timeout(),true);
    if(winrm == nil){
        return nil;
    }
    return winrm;
}

func wmi_query(wmi_handle,query,namespace=nil){
    var result = WMIQuery(wmi_handle,query,namespace);
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

func wmi_query_rsop(wmi_handle,query){
    return wmi_query(wmi_handle,query,"root\rsop\computer");
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
    var winrm = CreateWinRM(host,5985,username,password,false,true,"","","",get_default_connect_timeout(),true);
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

func registry_format_key(key, type=nil){
    if(!type){
        type = "HKLM";
    }
    var full = ToUpperString(type);
    if(key[0] != '\\'){
        full += "\\";
    }
    full += key;
    if(full[len(full)-1]=='\\'){
        full = full[:len(full)-1];
    }
    return full;
}

func registry_key_exists( key, type=nil, query_cache=nil, save_cache=nil){
    var full = registry_format_key(key,type);
    if(RegEnumKey(GetWinRM(),full) || RegEnumValue(GetWinRM(),full)){
        return true;
    }
    return false;
}

func registry_enum_keys( key, type=nil){
    var full = registry_format_key(key,type);
    var result = RegEnumKey(GetWinRM(),full);
    if(!result){
        return [];
    }
    var ret = [];
    var part = len(full)+1;
    for v in result{
        if(len(v) > part){
            ret += v[part:];
        }
    }
    return ret;
}

func registry_enum_keys_full( key, type=nil){
    var full = registry_format_key(key,type);
    var result = RegEnumKey(GetWinRM(),full);
    if(!result){
        return [];
    }
    return result;
}


func registry_enum_keys_with_deep(key,deep,type= nil){
    var list = registry_enum_keys_full(key,type);
    for(var i = 1; i <deep;i++){
        var temp = [];
        for v in list{
            var items = registry_enum_keys_full(v);
            for v in items{
                temp += v;
            }
        }
        list = temp;
    }
    return list;
}

func registry_get_sz(key, item, type=nil, multi_sz=nil, query_cache=nil, save_cache =nil ){
    var full = registry_format_key(key,type);
    var result = RegGetValue(GetWinRM(),full,item);
    if(result){
        if(!multi_sz){
            return result["value"];
        }
        return ReplaceAllString(result["value"],"\\0","\n");
    }
    return nil;
}

func registry_enum_values( key, type ){
    var full = registry_format_key(key,type);
    var result = RegEnumValue(WinRM(),full);
    if(!result){
        return [];
    }
    var list = [];
    for v in result{
        list += v["value"];
    }
    return list;
}

func registry_get_dword( key, item, type ){
    var full = registry_format_key(key,type);
    var result = RegGetValue(GetWinRM(),full,item);
    if(result){
        return ToInteger(result["value"]);
    }
    return nil;
}

func registry_get_binary( key, item, type ){
    var full = registry_format_key(key,type);
    var result = RegGetValue(GetWinRM(),full,item);
    if(result){
        return RegDecodeBinaryValue(result["value"]);
    }
    return nil;
}
#这个函数命名并不准确，其实是得到Windows目录
func smb_get_systemroot(){
    return GetWindowsDirectory();
}

func smb_get_system32root(){
    return GetSystemDirectory();
}

func registry_hku_subkeys(){
    return RegEnumKey(GetWinRM(),"HKEY_USERS");
}

func fetch_file_version( sysPath, file_name ){
    if(!sysPath || !file_name || len(sysPath) == 0 || len(file_name)==0){
        return "";
    }
    if(sysPath[len(sysPath)-1] != '\\' && file_name[0] != '\\'){
        sysPath += "\\";
    }
    sysPath += file_name;
    return GetFileVersion(GetWinRM(),sysPath);
}

func GetVersionFromFile( file, verstr="", offset=0 ){
    return GetFileVersion(file);
}

func get_version( dllPath, string="", offs=0 ){
    return GetFileVersion(file);
}

func GetVer( file, share=nil, prodvers=nil){
    return GetFileVersion(file);
}

func smb_read_file( share, file, fullpath, offset=0, count=0 ){
    if(!share && !file && !fullpath){
        return "";
    }
    if(!file){
        if(fullpath){
            file = fullpath;
        }else if(share){
            file = share;
        }
    }
    var data = GetFileContent(GetWinRM(),file);
    if(!data){
        return "";
    }
    if(offset > 0){
        data = data[offset:];
    }
    if (count > 0 && len(data) > count){
        return data[:count];
    }
    return data;
}
func get_file_size( share=nil, file ){
    var size = GetFileFileSize(GetWinRM(),file);
    if(!size){
        return 0;
    }
    return ToInteger(size);
}