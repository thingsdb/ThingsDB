package main

import (
	"log"
	timod "requests/timod"

	"github.com/vmihailenco/msgpack"
)

func onRequest(pkg *timod.Pkg) {
	type Request struct {
		URL string `msgpack:"url"`
	}
	var request Request

	err := msgpack.Unmarshal(pkg.Data, &request)
	if err == nil {
		timod.WriteResponse(pkg.Pid, &request)
	} else {
		timod.WriteEx(pkg.Pid, timod.ExBadData, "failed to unpack request")
	}
}

func handler(buf *timod.Buffer, quit chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch timod.Proto(pkg.Tp) {
			case timod.ProtoModuleInit:
				log.Println("No init required for this module")
			case timod.ProtoModuleReq:
				onRequest(pkg)
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
