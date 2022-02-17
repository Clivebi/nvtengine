#this file include all API implement in engine/modules,
#this file can't as a script running,only as a doc to the script dev


##below function implement in the engine,so can't overwrite by script

#print var to the stdout as one line
func Println(...)

#convert (integer/byte/double to one byte)
func byte(src)

#return the length of (bytes,string,array,map)
#when type of src not have length,throw exception
func len(src)

#create new copy of src,in script,the bytes,array,map,object is a reference type,tow var can use same memory
#this function crate a new copy use different memory
func clone(src)

#append src to the dst(bytes/array)
#when dst is bytes, src only accept byte,string,bytes 
#when dst is array ,src accept any type
func append(dst,src)

#src only accept array,map
#delete element from src at key(or index)
func delete(src,key)

#load file as script in curren context
#file is the script name
func require(file)

#accept zero or one parameter return bytes
#parameter type accept bytes,string
func bytes(src)


#create new bytes that fill with zero and length is size
func MakeBytes(size)

#close any system resource
#in the script,close the resource is not required,when the script terminate,
#the engine auto release the resource
func close(resource)

#return the type of src as string
func typeof(src)

#report error and exit script
func error(msg)

#convert any type to string
func string(src)

#same as string function
func ToString(src)

#convert any type to integer
func ToInteger(src)

#convert any type to float
func ToFloat(src)

#dump info of src to the stdout
func DumpVar(src)

#decode a hex string to bytes
func HexDecodeString(text)

#encode src to hex string
#src accept bytes/string/integer
func HexEncode(src)

#display total context (all var) in the stdout
#full true or false
#when full is true,display all information
func DisplayContext(full)

#return a map include all engine env
func VMEnv()

#return a short call stack 
func ShortStack()

#get all avaliable function,include engine C function and script function
func GetAvaliableFunction()

#check funcname is exist or not
func IsFunctionExist(funcname)


#below function implement in extend builtin modules,that can overwrite by script
#when script define a same name function,builtin function is being overwrited

#encode base64 
#src accept string/bytes
func StdBase64Encode(src)

#decode base64 string to string
#src accept string/bytes
func StdBase64Decode(src)

#check the find is in the src,the ***Bytes function have a name of ***String
#so ContainsBytes equal ContainsString
func ContainsBytes(src,find)

#check the src prefix with find or not
func HasPrefixBytes(src,find)

#check the src suffix with find or not
func HasSuffixBytes(src,find)

#remove all byte in the els from the left of src
#eg: TrimLeftBytes("\r\n hello","\r\n ") remove all '\r' '\n' and space from src
func TrimLeftBytes(src,els)

#remove all byte in the els from the right of src
func TrimRightBytes(src,els)

#remove all byte in the els from the right and left of src
func TrimBytes(src,els)

#return the first index of find in the src
#if not find ,return 1
func IndexBytes(src,find)

#return the last index of find in the src
#if not find ,return 1
func LastIndexBytes(src,find)

#create a string or bytes consisting of count copies of src
func RepeatBytes(src,count)

#return a string or bytes replace first n non-overlapping instances of old replaced by new
#when count = 1,replace all 
func ReplaceBytes(src,old,new,count)

#replace all equal ReplaceBytes(src,old,new,-1)
func ReplaceAllBytes(src,old,new)

#return a array,split src with sep ,the element in the result not include the sep
func SplitBytes(src,sep)

#lower case src
func ToLowerBytes(src)

func ToUpperBytes(src)

#check the buf can match regexp or not
#if icase is true,ignore case
func IsMatchRegexp(buf,regexp,icase)

#return a array,that all text match the regexp
func SearchRegExp(buf,regexp,icase)

#replace all text matched the regexp with new
func RegExpReplace(buf,regexp,new,icase)

#return a hex stirng as  hexdump -C result
func HexDumpBytes(buf)

#copy bytes,from src to dst
func CopyBytes(dst,src)


#do http get method and return result
#header is a map Contains key-value pair
func HttpGet(url,header(optional))

#do http post method and return result
func HttpPost(url,contentype,content,header(optional))

#do http post from
#values is a map of key-value pair
func HttpPostForm(url,values,header(optional))

#decode gzip 
func DeflateBytes(src)

#decode br
func BrotliDecompressBytes(src)

#parse url 
func URLParse(url)

#parse query stirng to key-value map
func URLQueryDecode(query)

#encode a key-value map to query string
func URLQueryEncode(values)

#encode str to url safe string for query string (UrlEncode)
func URLQueryEscape(str)
#for decode
func URLQueryUnescape(str)

#encode str to url safe string for path
func URLPathEscape(path)
#for decode
func URLPathUnescape(path)

#read response from con
func ReadHttpResponse(con)

#decode text to value (return array,or map)
func JSONDecode(str)

#encode value to json string
#if value is atom value ,just convert it to json safe string,not object
#if value is array or map,convert to a json array or object
func ToJSONString(value)

#connect the host at port use TCP,return the con
#if isSSL true,do SSL connect
#if isEnableCache is true,will use buffered connection that will reduce system read call count
func TCPConnect(host,post,timeout,isSSL,isEnableCache)

#connect the host at port use UDP,return the con
func UDPConnect(host,port)

#use a buffer that size equal length to read data from con,return the result
func ConnRead(con,length)

#use a buffer that size equal length to read data from con,return the result
func ConnRead(con,length)

#writer data to the con,if data not bytes or stirng,convert to string
func ConnWrite(con,data)

#read data until end with deadtext,if limitsize is 0,the limitsize is 4M
#if data size arrived the limitsize and not match deadtext,return the data
#we recommand use this function on a  cache enabled connection or 
func ConnReadUntil(con,deadtext,limitsize)

#performance a tls handle shake on con( only TCP)
func ConnTLSHandshake(con)

#set read write timeout,the defualt value is 3 second
func ConnSetReadTimeout(con,timeout)
func ConnSetWriteTimeout(con,timeout)

#return the peer cert on the TLS connection
#the result is a 'cert' type resource,user can use the cert API to query information
func ConnGetPeerCert(con)

#query cert information
#cmd: serial,issuer,subject,not-before,not-after,fpr-sha-256,fpr-sha-1,hostnames,algorithm-name
#cmd:image return the der data
#cmd:modulus RSA n
#cmd:exponent RSA e 
func X509Query(cert,cmd,index)

#return the cert form data
#the type indecate the encode type:"ASN1",or "PEM"
func X509Open(data,type)

#return a cipher resource
#cipherName such as: "rc4","aes-128-cbc","aes-256-cbc"
#padding:PKCS7 :1, ISO7816_4:2,ANSI923:3,ISO10126:4,ZERO:5
#key & IV ,the cipher key IV 
#isencrypt true for encrypt,false to decrypt
func CipherOpen(cipherName,padding,key,iv,isencrypt)

#update whith data and return the part of encrypt/decrypt data
#if the size of data not algin to the cipher block size,return the part of encrypt/decrypt data
func CipherUpdate(cipher,data)
#get the last block
func CipherFinal(cipher)

...all openvas implement API is also implemented