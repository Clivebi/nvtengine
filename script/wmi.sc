func _filter_item(item,keys){
    var ret = {};
    for v in keys{
        ret[v] = item[v];
    }
    ret["Properties"] = item["Properties"];
    return ret;
}

# Get-WmiObject -Query {SELECT Name from Win32_Processor}| Format-List -Property *
func WMIQuery(handle,query){
    var cmd = "Get-WmiObject -Query {"+query+"}"+"| Format-List -Property *";
    var result = WinRMCommand(handle,cmd,"",true);
    var temp = [];
    if (result.ExitCode != 0 || len(result.StdErr) > 0){
        Println(result);
        return nil;
    }
    var out = TrimString(result.Stdout,"\r\n\t ");
    var lines = SplitString(out,"\n");
    var item = {};
    for line in lines{
        line = TrimString(line,"\r\n\t ");
        var pos = IndexString(line,":");
        if(pos ==-1){
            continue;
        }
        var key =   line[:pos];
        var value = line[pos+1:];
        key = TrimString(key,"\r\n\t ");
        value = TrimString(value,"\r\n\t ");
        if(key=="PSComputerName"){
            if(len(item)){
                temp = append(temp,item);
                item = {};
            }
            continue;
        }
        if (HasPrefixString(key,"__")){
            continue;
        }
        item[key] = value;
    }
    temp = append(temp,item);
    var ret = [];
    for v in temp{
        var pros = TrimString(v["Properties"],"{}");
        if(!ContainsBytes(pros,"...")){
            ret = append(ret,_filter_item(v,SplitString(pros,", ")));
        }else{
            ret = append(ret,v);
        }
    }
    return ret;
}

func _print_wmi_result(keys,item){
    var result = "";
    for v in keys{
        if(len(result)){
            result += "|";
        }
        result += item[v];
    } 
    result += "\n";
    return result;
}

func wmi_query(handle,query){
    var result = WMIQuery(handle,query);
    if(len(result)==0){
        return "";
    }
    var pros = TrimString(result[0]["Properties"],"{}");
    var keys = [];
    if(!ContainsBytes(pros,"...")){
        keys = SplitString(pros,", ");
    }else{
        if (len(result)) {
            for k,v in result[0]{
                keys = append(keys,k);
            }
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