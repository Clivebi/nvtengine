require("win_warp.inc.sc");

func AssertNotEmpty(val){
    if(!val||len(val)==0){
        DisplayContext();
        error("Empty val "+val);
    }
}
func Assert(val){
    if(!val){
        DisplayContext();
        error("Value not TRUE",val);
    }
}

func AssertNot(val){
    if(val){
        DisplayContext();
        error("Value is TRUE",val);
    }
}

SetWinRMHandle(CreateWinRM("192.168.4.180",5985,"Lewis","Lewis123",false,true,"","","",60,true));

AssertNotEmpty(GetWindowsDirectory());
AssertNotEmpty(GetSystemDirectory());
AssertNotEmpty(GetWindowsBuildNumber());
AssertNotEmpty(GetWindowsCaption());
AssertNotEmpty(GetWindowsVersion());
Assert(IsPathExist("C:\\Windows"));
Assert(IsPathExist("C:\\Windows\\notepad.exe"));
AssertNot(IsPathExist("C:\\Windows.old"));
var content = GetFileContent(GetWinRM(),"C:\\windows\\notepad.exe");
Println("Size=",len(content));
Println(HexEncode(SHA256(content)));


Println(RegGetValue(GetWinRM(),"HKLM\\Software\\Microsoft\\Driver Signing","Policy"));
Println(wmi_reg_get_bin_val(GetWinRM(),"Software\\Microsoft\\Driver Signing","Policy"));
Println(wmi_reg_get_bin_val(GetWinRM(),"System\\CurrentControlSet\\Control\\Lsa","fullprivilegeauditing"));
Println(wmi_reg_enum_key(GetWinRM(),"Software\\7-Zip"));
Println(wmi_reg_enum_value(GetWinRM(),"Software\\7-Zip"));
Println(wmi_reg_get_sz(GetWinRM(),"Software\\7-Zip","Path"));
Println(wmi_reg_get_dword_val(GetWinRM(),"Software\\7-Zip","Test1"));
Println(wmi_reg_get_ex_string_val(GetWinRM(),"Software\\7-Zip","Test5"));
Println(wmi_reg_get_mul_string_val(GetWinRM(),"Software\\7-Zip","Test3"));
Println(wmi_reg_get_bin_val(GetWinRM(),"Software\\7-Zip","Test4"));
Println(RegEnumKey(GetWinRM(),"HKLM\\Software\\7-Zip"));
Println(RegEnumValue(GetWinRM(),"HKLM\\Software\\7-Zip"));
Println(RegGetValue(GetWinRM(),"HKLM\\Software\\7-Zip","Path"));
#Println(RegAddKey(GetWinRM(),"HKLM\\Software\\7-Zip\\Scaned\""));
#Println(RegAddValue(GetWinRM(),"HKLM\\Software\\7-Zip\\Scaned\"",{"name":"Test\"","type":"REG_SZ","value":"TestValue"}));
Println(wmi_query(GetWinRM(),"SELECT ProcessId,Name,ExecutablePath FROM Win32_Process"));
Println(wmi_query(GetWinRM(),"SELECT Name, TotalPhysicalMemory FROM Win32_Computersystem"));