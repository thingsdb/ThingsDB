package main

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"

	timod "github.com/thingsdb/go-timod"

	"github.com/vmihailenco/msgpack"
)

type reqData struct {
	URL     string      `msgpack:"url"`
	Method  string      `msgpack:"metohd"`
	Body    []byte      `msgpack:"body"`
	Headers [][2]string `msgpack:"headers"`
	Params  [][2]string `msgpack:"params"`
}

type resData struct {
	StatusCode int    `msgpack:"status_code"`
	Body       []byte `msgpack:"body"`
}

func handleReqData(pkg *timod.Pkg, data *reqData) {

	params := url.Values{}

	reqURL, err := url.Parse(data.URL)
	if err != nil {
		timod.WriteEx(
			pkg.Pid,
			timod.ExBadData,
			fmt.Sprintf("Error: Failed to parse URL (%s)", err))
		return
	}

	body := bytes.NewReader(data.Body)

	for i := 0; i < len(data.Params); i++ {
		param := data.Params[i]
		key, value := param[0], param[1]
		params.Set(key, value)
	}

	reqURL.RawQuery = params.Encode()

	req, err := http.NewRequest(data.Method, reqURL.String(), body)
	if err != nil {
		timod.WriteEx(
			pkg.Pid,
			timod.ExBadData,
			fmt.Sprintf("Error: Failed to create HTTP request (%s)", err))
		return
	}

	for i := 0; i < len(data.Headers); i++ {
		header := data.Headers[i]
		key, value := header[0], header[1]
		req.Header.Set(key, value)
	}

	client := &http.Client{}
	res, err := client.Do(req)
	if err != nil {
		timod.WriteEx(
			pkg.Pid,
			timod.ExOperation,
			fmt.Sprintf("Error: Failed to do the HTTP request (%s)", err))
		return
	}

	var response resData

	// Set the body
	response.Body, err = ioutil.ReadAll(res.Body)
	if err != nil {
		timod.WriteEx(
			pkg.Pid,
			timod.ExBadData,
			fmt.Sprintf("Error: Failed to read bytes from HTTP response (%s)", err))
		return
	}

	// Set the status code
	response.StatusCode = res.StatusCode

	timod.WriteResponse(pkg.Pid, &response)
}

func onModuleReq(pkg *timod.Pkg) {
	var data reqData

	err := msgpack.Unmarshal(pkg.Data, &data)
	if err == nil {
		handleReqData(pkg, &data)
	} else {
		timod.WriteEx(
			pkg.Pid,
			timod.ExBadData,
			fmt.Sprintf("Error: Failed to unpack request (%s)", err))
	}
}

func handler(buf *timod.Buffer, quit chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch timod.Proto(pkg.Tp) {
			case timod.ProtoModuleConf:
				log.Println("No configuration data is required for this module")
				timod.WriteConfOk()
			case timod.ProtoModuleReq:
				onModuleReq(pkg)
			default:
				log.Printf("Error: Unexpected package type: %d", pkg.Tp)
			}
		case err := <-buf.ErrCh:
			log.Printf("Error: %s", err)
			quit <- true
			return
		}
	}
}

func main() {
	timod.StartModule("requests", handler)
}
