// Demo is a ThingsDB module which may be used as a template to build modules.
//
// This module simply extract a given `message` property from a request and
// returns this message.
//
// For example:
//
//     // Create the module (@thingsdb scope)
//     new_module('DEMO', 'demo', nil, nil);
//
//     // When the module is loaded, use the module in a future
//     future({
//       module: 'DEMO',
//       message: 'Hi ThingsDB module!',
//     }).then(|msg| {
//	      `Got the message back: {msg}`
//     });
//
package main

import (
	"fmt"
	"log"
	"sync"

	timod "github.com/thingsdb/go-timod"

	siridb "github.com/SiriDB/go-siridb-connector"
	"github.com/vmihailenco/msgpack"
)

var client *siridb.Client = nil
var mux sync.Mutex

type serverSiriDB struct {
	Host string `msgpack:"host"`
	Port int    `msgpack:"port"`
}

type authSiriDB struct {
	Username string         `msgpack:"username"`
	Password string         `msgpack:"password"`
	Database string         `msgpack:"database"`
	Servers  []serverSiriDB `msgpack:"servers"`
}

type reqSiriDB struct {
	Type    string `msgpack:"type"`
	Query   string `msgpack:"query"`
	Timeout uint16 `msgpack:"timeout"`
}

func printLogs(logCh chan string) {
	for {
		msg := <-logCh
		fmt.Printf("Log: %s\n", msg)
	}
}

func handleConf(auth *authSiriDB) {
	mux.Lock()
	defer mux.Unlock()

	if client != nil {
		client.Close()
	}

	servers := make([][]interface{}, len(auth.Servers))

	for i := 0; i < len(auth.Servers); i++ {
		s := []interface{}{auth.Servers[i].Host, auth.Servers[i].Port}
		servers[i] = s
	}

	client = siridb.NewClient(
		auth.Username,
		auth.Password,
		auth.Database,
		servers,
		nil,
	)

	client.Connect()

	timod.WriteConfOk()
}

func handleQuery(pkg *timod.Pkg, req *reqSiriDB) {
	if req.Timeout == 0 {
		req.Timeout = 10
	}
	res, err := client.Query(req.Query, req.Timeout)
	if err != nil {
		timod.WriteEx(
			pkg.Pid,
			timod.ExOperation,
			fmt.Sprintf("Query has failed: %s", err))
		return
	}
	timod.WriteResponse(pkg.Pid, &res)
}

func handleReq(pkg *timod.Pkg, req *reqSiriDB) {
	switch req.Type {
	case "QUERY":
		handleQuery(pkg, req)
		return
	case "INSERT":
	default:
		timod.WriteEx(
			pkg.Pid,
			timod.ExBadData,
			fmt.Sprintf("Error: Invalid type: %s", req.Type))
	}
}

func onModuleReq(pkg *timod.Pkg) {
	mux.Lock()
	defer mux.Unlock()

	if client == nil || !client.IsConnected() {
		timod.WriteEx(
			pkg.Pid,
			timod.ExOperation,
			"Error: SiriDB is not connected; please check the module configuration")
		return
	}

	var req reqSiriDB
	err := msgpack.Unmarshal(pkg.Data, &req)
	if err != nil {
		timod.WriteEx(
			pkg.Pid,
			timod.ExBadData,
			"Error: Failed to unpack SiriDB request")
	}
	handleReq(pkg, &req)
}

func handler(buf *timod.Buffer, quit chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch timod.Proto(pkg.Tp) {
			case timod.ProtoModuleConf:
				var auth authSiriDB
				err := msgpack.Unmarshal(pkg.Data, &auth)
				if err == nil {
					handleConf(&auth)
				} else {
					log.Println("Error: Failed to unpack SiriDB configuration")
					timod.WriteConfErr()
				}

			case timod.ProtoModuleReq:
				onModuleReq(pkg)

			default:
				log.Printf("Error: Unexpected package type: %d", pkg.Tp)
			}
		case err := <-buf.ErrCh:
			// In case of an error you probably want to quit the module.
			// ThingsDB will try to restart the module a few times if this
			// happens.
			log.Printf("Error: %s", err)
			quit <- true
		}
	}
}

func main() {
	// Starts the module
	timod.StartModule("siridb", handler)

	if client != nil {
		client.Close()
	}
}
