require("nasl.sc");

func SSH_shell_test(session){
   var shell = ssh_shell_open(session);
   if(!shell){
      Println("ssh_shell_open failed");
      return;
   }
   if(ssh_shell_write(shell,"ps -A\n") != 0){
      Println("ssh_shell_write failed");
      return;
   }
   for (var i =0; i < 10;i++){
      var text = ssh_shell_read(shell);
      if(text != nil){
         Println(HexDumpBytes(text));
      }
      sleep(1);
   }
}

func SSH_test(ip,login,password){
   var socket = TCPConnect(ip,22,15,false,false);
   if(socket == nil){
      Println("Connect Failed:",ip+":"+22);
      return;
   }
   var session = ssh_connect(socket,"",ip);
   if(session == nil){
      Println("ssh_connect failed");
      return;
   }
   if(socket != ssh_get_sock(session)){
      Println("ssh_get_sock not equal");
      ssh_disconnect(session);
      return;
   }
   if(session != ssh_session_id_from_sock(socket)){
      Println("ssh_session_id_from_sock not equal");
      ssh_disconnect(session);
      return;
   }

   Println("pubkey:",ssh_get_host_key(session));
   Println("server_banner:",ssh_get_server_banner(session));
   Println("issue_banner:",ssh_get_issue_banner(session,login));
   Println("auth methods:",ssh_get_auth_methods(session,login));

   var result = ssh_userauth(session,login,password);
   if(result < 0){
      Println("ssh_userauth failed ",result);
      ssh_disconnect(session);
      return;
   }
   Println("issue_banner:",ssh_get_issue_banner(session,login));
   var result = ssh_request_exec(session,"ps -A");
   Println("ps result:",result);

   SSH_shell_test(session);
   ssh_disconnect(session);
}
SSH_test("192.168.3.72","xxx","xxx");