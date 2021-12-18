
#host,port,login,password,ishttps,inscure,ca(base64 encode),cert(base64 encode),certkey,timeout( second).useNTLM
var winrm = CreateWinRM("192.168.4.180",5985,"xx","xx",false,true,"","","",15,true);
if(winrm == nil){
    Println("CreateWinRM failed...");
    exit(0);
}

var result = WinRMCommand(winrm,"REG QUERY HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall","");
Println(result);