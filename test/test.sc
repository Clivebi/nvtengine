
#test help function

Println(VMEnv());

func list_builtin_function(){
    var list = GetAvaliableFunction();
    var list = list["builtin"];
    for v in list{
        Println(v);
    }
}
list_builtin_function();
var _is_test_passed = true;
func assertEqual(a,b){
    if(a != b){
        Println("error here a=",a,"b=",b);
        DisplayContext();
        _is_test_passed = false;
    }
}

func assertNotEqual(a,b){
    if(a == b){
        Println("error here a=",a,"b=",b);
        DisplayContext();
        _is_test_passed = false;
    } 
}

#basic arithmetic operation

assertEqual(1+1+2,4);
assertEqual(1+(1+2),4);
assertEqual(1+2*4,9);
assertEqual(4*(1+2),12);
assertEqual(4*5/5,4);
assertEqual(0-1000,-1000);
assertEqual(5*(-10),-50);
assertEqual(nil,false);
assertNotEqual(true,nil);
assertNotEqual(true,false);
func test_basic_convert(){
    var i = 100,f = 3.1415;
    var res = i+f;
    assertEqual(typeof(i),"integer");
    assertEqual(typeof(f),"float");

    #float + integer = float
    assertEqual(typeof(res),"float");

    #integer+=float  result as float
    i += 4.6;
    assertEqual(typeof(i),"float");

    #convert string to bytes
    var buf = bytes("hello");
    var buf2 = bytes(" world");

    assertEqual(typeof(buf),"bytes");
    assertEqual(typeof(buf2),"bytes");

    #convert hex string to bytes
    var buf5= HexDecodeString("68656C6C6F20776F726C64");
    var buf6= append(buf,buf2);

    assertEqual(buf5,buf6);
    Println(buf6);

    #convert bytes to string 
    var strTemp = ToString(buf5);
    assertEqual(typeof(strTemp),"string");
    assertEqual(strTemp,"hello world");

    #convert string to integer & float
    assertEqual(ToInteger("0xEEFFFF"),0xEEFFFF);
    assertEqual(ToInteger("8000"),8000);
    assertEqual(ToFloat("3.1415"),3.1415);
    assertEqual(ToFloat("3"),3.0);
    assertEqual(ToString(1000),"1000");
    assertEqual(ToString(3.14),ToString(ToFloat("3.14")));
    assertEqual(HexEncode(0xEEFFBB),"EEFFBB");
    assertEqual(100+"200","100200");
    assertEqual("100"+200,"100200");
}

test_basic_convert();

func asciiCodeToChar(code){
    return ToString(byte(code));
}

assertEqual(asciiCodeToChar('A'),"A");

func test_bitwise_operation(){
    assertEqual(0xFFEE & 0xFF,0xEE);
    assertEqual(0xFF00 | 0xFF,0xFFFF);
    assertEqual(0xFF ^ 0x00,0xFF);
    assertEqual(0xFF54>>8,0xFF);
    assertEqual(0xFF54<<8,0xFF5400);
    assertEqual(true || false,true);
    assertEqual(false || true,true);
    assertEqual(false && true,false);
    assertEqual(true && false,false);
    assertEqual(true && true,true);
}

test_bitwise_operation();


func test_loop(){
    var j =0,k=0;
    for (var i = 0;i< 9;i++){
        j++;
        k++;
    }
    assertEqual(j,9);
    assertEqual(k,9);

    j=0;
    k=0;
    for (var i = 0;i< 9;i++){
        j++;
       if(!(i%2)){
           k++;
       }
    }
    assertEqual(k,5);
    assertEqual(j,9);
    j=0;
    k=0;
    for (var i = 0;i< 9;i++){
        j++;
        if(i > 5){
            continue;
        }
        k++;
    }
    assertEqual(k,6);
    assertEqual(j,9);

    j=0;
    k=0;
    for (var i = 0;i< 20;i++){
        j++;
        if(i >= 8){
            break;
        }
        k++;
    }
    assertEqual(k,8);
    assertEqual(j,9);

    j=0;
    k=0;
    for{
        j++;
        k++;
        if(j > 8){
            break;
        }
    }
    #empty body
    for(var i =0;i<3;i++){

    }
    assertEqual(k,9);
    assertEqual(j,9);
}

test_loop();

func test_shadow_name(name1){
    var name2 = name1;
    for(var name2 =0; name2< 3;name2++){
    }
    for(var i =0; i< 3;i++){
        name2++;
    }
    assertNotEqual(name1,name2);
}

test_shadow_name(100);

func test_array_map(){
    var list = ["hello"," ","world"];
    assertEqual(typeof(list),"array");
    assertEqual(len(list),3);
    assertEqual(list[0],"hello");
    list = append(list,"script");
    assertEqual(len(list),4);
    assertEqual(list[3],"script");
    list = append(list,100);
    assertEqual(len(list),5);
    assertEqual(typeof(list[4]),"integer");

    var sub = list[2:];
    assertEqual(sub[0],"world");
    assertEqual(len(sub),3);
    
    sub = list[2:4];
    assertEqual(len(sub),2);
    assertEqual(sub[0],"world");
    assertEqual(sub[1],"script");
    var _dirs = ["/bin/","/usr/bin/","/usr/local/bin/"];
    var _files = ["test1","test2","test3"];
    var _full_paths = [];
    var full;
    for v in _dirs{
        for v2 in _files{
         _full_paths = append(_full_paths, v+v2);
        }
    }
    assertEqual(len(_full_paths),len(_dirs)*len(_files));

    var dic = {"accept":"text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9",
     "accept-encoding":"gzip, deflate, br",
     "referer":"https://www.google.com/",
     "accpet-languare":"zh-CN,zh;q=0.9,en;q=0.8",
     "X":1000
    };
    
    dic[800]= "800";

    assertEqual(typeof(dic),"map");
    assertEqual(len(dic),6);
    assertEqual(dic["X"],1000);
    dic["800"]= "800";
    assertEqual(dic["800"],"800");
    assertEqual(dic["accept-encoding"],"gzip, deflate, br");

    var dic2 ={100:"100",200:"200",300:"300",400:"400",3.14:3.1415926};
    assertEqual(dic2[100],"100");
    assertEqual(dic2[3.14],3.1415926);
}

func map_test_ex(){
    var dic = {};
    var dic2 = {"name":"wawa","type":"aaaa",100:"459","100":"900"};
    dic["test"] = dic2;
    assertEqual(dic2[100],"459");
    assertEqual(dic2["100"],"900");
    dic2[3.1415926]= "pi";
    Println(dic2);
    Println(dic);
}
map_test_ex();

test_array_map();

# ByteOrder test
func blob_test(){
    var blob = MakeBytes(10);
    for (var i = 0; i< 10;i++){
        blob[i] = '0'+i;
    }
    assertEqual(len(blob[:2]),2);
    assertEqual(string(blob[:2]),"01");
    var x = blob[4:];
    assertEqual(len(x),6);
    assertEqual(string(x[:2]),"45");
}

blob_test();

func byteslib_test(){
    var str = "!!! hello world !!!!!";
    result = TrimString(str,"! ");
    assertEqual(result,"hello world");
    result = TrimLeftString(str,"! ");
    assertEqual(result,"hello world !!!!!");
    result = TrimRightString(str,"! ");
    assertEqual(result,"!!! hello world");
    assertEqual(ContainsString(str,"hello"),true);
    assertEqual(IndexString(str,"hello"),4);
    assertEqual(IndexString(str,"helloW"),-1);
    assertEqual(LastIndexString(str,"o"),11);
    assertEqual(ReplaceAllString(str,"!!! ",""),"hello world !!!!!");
    assertEqual(ReplaceString(str,"!","",3)," hello world !!!!!");
    assertEqual(ReplaceString(str,"!","",-1),ReplaceAllString(str,"!",""));
    var list = RepeatString("hello ",5);
    var list_array = SplitString(TrimString(list," ")," ");
    assertEqual(len(list_array),5);
    for v in list_array{
        assertEqual(v,"hello");
    }
    assertEqual(ToLowerString("heLLo"),"hello");
    assertEqual(ToUpperString("heLLo"),"HELLO");
    assertEqual(HasPrefixString(str,"!!! X"),false);
    assertEqual(HasSuffixString(str," !!!!!"),true);
    assertEqual(HasPrefixString(str,"!!! "),true);
    assertEqual(HasSuffixString(str," X!!!!!"),false);
}

byteslib_test();

if(_is_test_passed){
    Println("all test passed");
}else{
    Println("some test not passed");
}