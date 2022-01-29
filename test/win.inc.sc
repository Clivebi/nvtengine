require("nasl.sc");

#
# API implement

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

func IsWindowsHostAccessable(){
    var winrm = GetWinRM();
    return winrm != nil;
}

#query system information 
func GetOperatingSystemInformation(){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
    var infos = WMIQuery(GetWinRM(),"SELECT * FROM Win32_OperatingSystem");
    if(!infos){
        return nil;
    }
    if(typeof(infos)=="array" && len(infos)){
        return infos[0];
    }
    return nil;
}

func GetWindowsDirectory(){
    var info = GetOperatingSystemInformation();
    if(info){
        return info.WindowsDirectory;
    }
    return "";
}

func GetSystemDirectory(){
    var info = GetOperatingSystemInformation();
    if(info){
        return info.SystemDirectory;
    }
    return "";
}

func GetWindowsBuildNumber(){
    var info = GetOperatingSystemInformation();
    if(info){
        return info.BuildNumber;
    }
    return "";
}

func GetWindowsCaption(){
    var info = GetOperatingSystemInformation();
    if(info){
        return info.Caption;
    }
    return "";
}

func GetWindowsVersion(){
    var info = GetOperatingSystemInformation();
    if(info){
        return info.Version;
    }
    return "";
}


#query process information 
func GetProcessInformation(){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
    return WMIQuery(GetWinRM(),"SELECT * FROM Win32_Process");
}

#query services information 
func GetServicesInformation(){
    # Win32_Service
    if(!IsWindowsHostAccessable()){
        return nil;
    }
    return WMIQuery(GetWinRM(),"SELECT * FROM Win32_Service");
}

func _warp_single_qute_string(str){
    str = ReplaceAllString(str,"\\","\\\\");
    return "'"+str+"'";
}

func GetFileInformation(filepath){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
    var wql = "SELECT * FROM CIM_DataFile WHERE Name="+_warp_single_qute_string(filepath);
    return WMIQuery(GetWinRM(),wql);
}

#File directory API
func GetFileVersion(filepath){
    info = GetFileInformation(filepath);
    if(!info || len(info) == 0){
        return nil;
    }
    return info[0]["Version"];
}

func GetFileFileSize (filepath){
    info = GetFileInformation(filepath);
    if(!info || len(info) == 0){
        return nil;
    }
    return info[0]["FileSize"];
}

func GetFileFileType (filepath){
    info = GetFileInformation(filepath);
    if(!info || len(info) == 0){
        return nil;
    }
    return info[0]["FileType"];
}

func EnumChildDirectoryFull(dir){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
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
    return WMIQuery(GetWinRM(),wql);
}

func EnumChildDirectoryWithLightInformation(dir){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
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
    return WMIQuery(GetWinRM(),wql);
}

func EnumChildFileFull(dir){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
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
    return WMIQuery(GetWinRM(),wql);
}

func EnumChildFileWithLightInformation(dir){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
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
    return WMIQuery(GetWinRM(),wql);
}


func GetDirectoryInformation(dir){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
    var wql = "SELECT * FROM Win32_Directory WHERE Name="+_warp_single_qute_string(dir);
    var info = WMIQuery(GetWinRM(),wql);
    if(info && len(info)){
        return info[0];
    }
    return nil;
}

func IsFileExist(file){
    return GetFileFileType(file) != nil;
}

func IsPathExist(path){
    if(IsFileExist(path)){
        return true;
    }
    if(GetDirectoryInformation(path)){
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
    var list = EnumDirectoryWithLightInformation(path);
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
    var list = EnumChildFileWithLightInformation(path);
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

func GetFileContent(path){
    var cmd = "[System.Convert]::ToBase64String([System.IO.File]::ReadAllBytes("+_warp_single_qute_string(path) + "))";
    var result = WinRMCommand(GetWinRM(),cmd,"",true);
    if(!result){
        return nil;
    }
    if (result.ExitCode != 0 || len(result.StdErr) > 0){
        Println(result);
        return nil;
    }
    return StdBase64Decode(result.Stdout);
}





## test code



func PrintlnObject(val){
    if (typeof(val) == "array"){
        for v in val{
            PrintlnObject(v);
            Println("*****************************");
        }
        return;
    }
    for k,v in val{
        Println(k,":",v);
    }
}

func test_dir(){
    var infos = EnumChildDirectoryWithLightInformation("C:\\Windows");
    for v in infos{
        Println(v.FileType,"\t\t",v.FileSize,"\t",GetFileBaseName(v.Name),"\t");
    }
    var infos = EnumChildFileWithLightInformation("C:\\Windows");
    for v in infos{
        Println(v.FileType,"\t\t",v.FileSize,"\t",GetFileBaseName(v.Name),"\t");
    }
}
#var content = GetFileContent("C:\\windows\\notepad.exe");
#Println("Size=",len(content));
#Println(HexEncode(SHA256(content)));

#Println(HexDumpBytes(content));
#test_dir();
#Println(IsPathExist("C:\\Windows.old"));
#Println(IsPathExist("C:\\Windows"));
#Println(IsPathExist("C:\\Windows\\notepad.exe"));
#Println(GetSubDirectoryNameList("C:\\windows"));

#PrintlnObject(GetFileInformation("C:\\Windows\\notepad.exe"));
#Println(GetFileVersion("C:\\Windows\\notepad.exe"));
#PrintlnObject(GetOperatingSystemInformation());
Println(GetWindowsCaption(),GetWindowsVersion());

#PrintlnObject(GetProcessInformation());

#PrintlnObject(GetServicesInformation());