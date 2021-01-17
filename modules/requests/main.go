package main

import (
	"log"
	"os"
	timod "requests/timod"
)

func handler(buf *timod.Buffer, quit chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch timod.Proto(pkg.Tp) {
			case timod.ProtoModuleInit:
				log.Println("Initializing requests module")
			case timod.ProtoModuleReq:
				log.Println("Received a request")
				data, err := timod.PkgPack(0, 67, nil)
				if err == nil {
					log.Println("Write a response")
					os.Stdout.Write(data)
				}
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
