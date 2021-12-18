package main

//#include <stdio.h>
import (
	"C"
)
import (
	"encoding/base64"
	"runtime/cgo"
	"time"

	"github.com/masterzen/winrm"
)

//export WinRMOpen
func WinRMOpen(host *C.char, port C.int, login *C.char,
	key *C.char, isHttps C.int, Inscure C.int, base64CA *C.char,
	base64Cert *C.char, certKey *C.char, timeout_second C.int, useNTLM C.int) uintptr {
	var caBuf, certBuf, keyBuf []byte
	ghost := C.GoString(host)
	username := C.GoString(login)
	password := C.GoString(key)
	httpsMode := false
	inscureMode := false
	if isHttps == 1 {
		httpsMode = true
	}
	if Inscure == 1 {
		inscureMode = true
	}
	inputCA := C.GoString(base64CA)
	inputCert := C.GoString(base64Cert)
	inputCertKey := C.GoString(certKey)

	if len(inputCA) > 0 {
		caBuf, _ = base64.StdEncoding.DecodeString(inputCA)
	}
	if len(inputCert) > 0 {
		certBuf, _ = base64.RawStdEncoding.DecodeString(inputCert)
	}
	if len(inputCertKey) > 0 {
		keyBuf = []byte(inputCertKey)
	}

	endpoint := winrm.NewEndpoint(ghost, int(port), httpsMode, inscureMode,
		caBuf, certBuf, keyBuf, time.Duration(timeout_second)*time.Second)
	if useNTLM == 1 {
		params := winrm.DefaultParameters
		params.TransportDecorator = func() winrm.Transporter { return &winrm.ClientNTLM{} }
		client, err := winrm.NewClientWithParameters(endpoint, username, password, params)
		if err != nil {
			return 0
		}
		return uintptr(cgo.NewHandle(client))
	}
	client, err := winrm.NewClient(endpoint, username, password)
	if err != nil {
		return 0
	}
	return uintptr(cgo.NewHandle(client))
}

//export WinRMClose
func WinRMClose(h uintptr) {
	cgo.Handle(h).Delete()
}

//export WinRMExecute
func WinRMExecute(handle uintptr, cmd *C.char, input *C.char) (StdOut *C.char, StdErr *C.char, ExitCode C.int, ErrorMsg *C.char) {
	client := cgo.Handle(handle).Value().(*winrm.Client)
	cmdstr := C.GoString(cmd)
	inputstr := C.GoString(input)
	out, errOut, code, err := client.RunWithString(cmdstr, inputstr)
	if err != nil {
		ErrorMsg = C.CString(err.Error())
	} else {
		StdOut = C.CString(out)
		StdErr = C.CString(errOut)
		ExitCode = C.int(code)
	}
	return
}

func main() {

}
