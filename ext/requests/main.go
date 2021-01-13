package main

import (
	"log"
	tiext "requests/tiext"
)

func handler(buf *tiext.Buffer, quit chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch tiext.Proto(pkg.Tp) {
			case tiext.ProtoExtInit:
				log.Println("Initializing requests extension")
			case tiext.ProtoExtClose:
				log.Println("Closing requests extension")
				quit <- true
				return
			case tiext.ProtoExtReq:
				log.Println("Received a request")
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
	buf := tiext.NewBuffer()

	go buf.Listen()

	quit := make(chan bool)
	go handler(buf, quit)
	<-quit
}
