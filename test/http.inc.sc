var _MAX_HTTP_REDIRECT_COUNT = 6;
var _DEFAULT_HTTP_HEADER = {};
_DEFAULT_HTTP_HEADER ={"Connection":"close"};
_DEFAULT_HTTP_HEADER["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,application/json, text/plain, */*";
_DEFAULT_HTTP_HEADER["Accept-Encoding"] = "gzip, deflate, br";
_DEFAULT_HTTP_HEADER["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8";
_DEFAULT_HTTP_HEADER["User-Agent"]="Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36";


func line_to_value_pair(line,split){
    var pos = IndexString(line,split);
    if(pos < 0 || pos >= (len(line)-1)){
        return nil;
    }
    return[line[0:pos],line[pos+1:]];
}

#parse http response set-Cookie into a map object
# result = [{"value":"xxxxxx=kxjxjj","path":"/",....}]
#
func parse_set_cookies(httpResponse){
    var i,v,v2;
    var setCookies = httpResponse["headers"]["Set-Cookie"];
    var fieldType = typeof(setCookies);
    var result =[];
    if(fieldType != "array"){
        return nil;
    }
    var splitResult,temp,item;
    for v in setCookies{
        item = {};
        splitResult = SplitString(v,";");
        for i,v2 in splitResult{
            if(i==0){
                item["value"] = v2;
                continue;
            }
            temp = line_to_value_pair(v2,"=");
            if(temp != nil){
                item[temp[0]] = temp[1];
            }
        }
        result = append(result,item);
    }
    return result;
}

func http_get_with_handle_redirect_internal(fromURL,header,redirect_count){
    if(header == nil){
        header = _DEFAULT_HTTP_HEADER;
    }
    if(redirect_count > _MAX_HTTP_REDIRECT_COUNT){
        return nil;
    }
    var resp =  HttpGet(fromURL,header);
    if(resp.status == 302){
        var location = resp.headers.Location;
        location = location[0];
        if(location != nil && len(location) >0){
            var cookiejar = parse_set_cookies(resp);
            if(cookiejar != nil){
                var cookies = "",v;
                for v in cookiejar{
                    if(len(cookies)){
                        cookies += "; ";
                    }
                    cookies += v.value;
                }
                header["Cookies"] = cookies;
            }
            #Println(fromURL,"-->",location);
            return http_get_with_handle_redirect_internal(location,header,redirect_count+1);
        }
    }
    return resp;
}

#http_get_with_handle_redirect
# HttpGet auto redirect version 
#fromURL the start url
#header addtional header field 
func http_get_with_handle_redirect(fromURL,header){
    return http_get_with_handle_redirect_internal(fromURL,header,0);
}
