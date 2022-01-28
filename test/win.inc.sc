require("nasl.sc");

#
# API implement

func GetWinRM(){
    var winrm = get_shared_object("winrm_handle");
    if(!winrm){
        winrm = CreateWinRM("192.168.4.180",5985,"Lewis","Lewis123",false,true,"","","",15,true);
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


func GetProcessInformation(){
    if(!IsWindowsHostAccessable()){
        return nil;
    }
    return WMIQuery(GetWinRM(),"SELECT * FROM Win32_Process");
}

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