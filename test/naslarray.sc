
func __index_nil__(index){
	DisplayContext(false);
	Println("convet nil object to NASLArray");
	return NASLArray();
}

object NASLArray(hash_table={},vector={}){
	func __len__(){
		var res = 0;
		for v in hash_table{
			res += len(v);
		}
		return res + len(vector);
	}
	func __get_index__(key){
		if(typeof(key) == "integer"){
			if(key<len(self.vector)){
				return self.vector[key];
			}
			return nil;
		}else{
			var list = self.hash_table[key];
			if(typeof(list)!= "array"){
				self.hash_table[key] = [];
				list = [];
			} 
			if (len(list)==0){
				return nil;
			}
			return list[len(list)-1];
		}
	}

	func __set_index__(key,value){
		if(typeof(key) == "integer"){
			self.vector[key] = value;
			return value;
		}
		var list = self.hash_table[key];
		if(typeof(list) != "array"){
			list = [];
		}
		if(len(list) == 0){
			list = append(list,value);
		}else{
			list[len(list)-1] = value;
		}
		self.hash_table[key] = list;
		return value;
	}

	func __enum_all__(){
		var result = [];
		DumpVar(self.vector);
		for k,v in self.vector{
            DumpVar(k,v);
            result = append(result,{"__key__":k,"__value__":v});
		}
		for k,v in self.hash_table{
			for v2 in v{
				result = append(result,{"__key__":k,"__value__":v2});
			}
		}
		return result;
	}

	func add_var_to_list(val){
		if(typeof(val)== "NASLArray" ||typeof(val)== "array" ||typeof(val)== "map"){
			for v in val{
				self.add_var_to_list(v);
			}
			return;
		}
		self.vector[len(self.vector)] = val;
	}

	func add_var_to_array(index,val){
		self.__set_index__(index,val);
	}
}

var res = NASLArray();
res[100] = "100";
for k,v in res{
	DumpVar(k,v);
}
res3["100"] = 100;
Println("res3",res3);