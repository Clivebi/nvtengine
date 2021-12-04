require("test.sc");
require("http.inc.sc");

func httplib_test(){
    var header ={"Connection":"close"};
    header["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9";
    header["Accept-Encoding"] = "gzip, deflate, br";
    header["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8";
    header["User-Agent"]="Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36";
    var url = "https://www.baidu.com/s?ie=utf-8&f=3&rsv_bp=1&rsv_idx=1&tn=baidu&wd=%E5%8D%8E%E5%B0%94%E8%A1%97%E4%B9%8B%E7%8B%BC&fenlei=256&rsv_pq=f5e3136d0000b48d&rsv_t=9057rH%2BDWhwEvA8EakAD8PvR8uKU%2B5BCRvFXZWPBwYB%2FqUWWnEJQ6RmSyBg&rqlang=cn&rsv_enter=1&rsv_dl=ih_0&rsv_sug3=2&rsv_sug1=2&rsv_sug7=001&rsv_sug2=1&rsv_btype=i&rsp=0&rsv_sug9=es_2_1&rsv_sug4=1205&rsv_sug=9";
    dic = URLParse(url);
    assertEqual(dic["host"],"www.baidu.com");
    assertEqual(dic["port"],"443");
    assertEqual(dic["scheme"],"https");
    assertEqual(dic["path"],"/s");
    var query ={"wd":"华尔街之狼","encode":"utf8","time":5000};
    assertEqual(URLQueryEncode(query),"encode=utf8&time=5000&wd=%E5%8D%8E%E5%B0%94%E8%A1%97%E4%B9%8B%E7%8B%BC");
    var resp = HttpGet("https://www.baidu.com/",header);
    assertEqual(resp["status"],200);
    assertEqual(resp["reason"],"OK");
    assertEqual(resp["version"],"HTTP/1.1");

    url = "http://api.k780.com/?app=weather.today&weaId=1&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json";
    query = {"app":"weather.today","weaId":1,"appkey":10003,"sign":"b59bc3ef6191eb9f747dd4e83c99f2a4","format":"json"};
    #resp = HttpPostForm("http://api.k780.com/",query,header);
    #resp = HttpGet(url,header);
    resp = HttpPost("http://api.k780.com/","application/x-www-form-urlencoded",URLQueryEncode(query),header);
    var info = JSONDecode(resp["body"]);
    Println(info["result"]["temperature"]);
    Println(ToString(resp["body"]));

}
httplib_test();

#https://192.168.0.100/restgui/locale/strings/locale_str_zh.json
#https://192.168.0.100/sysmgmt/2015/bmc/info
#https://192.168.0.105/session?aimGetProp=hostname,gui_str_title_bar,OEMHostName,fwVersion,sysDesc

func do_http_request(Url){
        return http_get_with_handle_redirect(Url,nil);
}

func detect_idrac(host,port){
    var result = {};
    var url = "https://"+host;
    if(port != 80 && port != 443){
        url +=port;
    }
    var resp = do_http_request(url+"/restgui/locale/strings/locale_str_en.json");
    if(resp.status == 200){
        if(ContainsString(resp["body"],"Integrated Dell Remote Access Controller 9")){
            resp = do_http_request(url+"/sysmgmt/2015/bmc/info");
            if(resp.status == 200){
                var info=JSONDecode(resp["body"]);
                result["AppName"] = "Integrated Dell Remote Access Controller 9";
                result["Version"] = info.Attributes.FwVer;
                result["UniqueName"] ="IDRAC9";
                result["Name"] = info.Attributes.iDRACName;
                result["Model"]=info.Attributes.SystemModelName;
                return result;
            }
        }
    }
    var generation = "7";
    resp = do_http_request(url+"/data?get=prodServerGen");
    if(resp["status"] == 200){
        if(ContainsString(resp["body"],"13G")){
            generation ="8";
        }else if(ContainsString(resp["body"],"12G")){
            generation = "7";
        }else{
            generation = "other";
        }
    }
    resp = do_http_request(url+"/session?aimGetProp=hostname,gui_str_title_bar,OEMHostName,fwVersion,sysDesc");
    if(resp["status"] == 200){
        var info = JSONDecode(resp["body"]);
        result["AppName"] = "Integrated Dell Remote Access Controller "+generation;
        result["Version"] = info["aimGetProp"]["fwVersion"];
        result["UniqueName"] ="IDRAC "+generation;
        result["Name"] = info["aimGetProp"]["hostname"];
        result["Model"]=info["aimGetProp"]["sysDesc"];
        return result;
    }
    return result;
}

func test_detect_idar(){
    var detect_result = detect_idrac("192.168.0.100",443);
    Println(detect_result);

    detect_result = detect_idrac("192.168.0.105",443);
    Println(detect_result);

    detect_result = detect_idrac("192.168.0.106",443);
    Println(detect_result);
}

test_detect_idar();

#var  lastResult = do_http_request("http://www.baidu.com/");