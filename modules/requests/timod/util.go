package timod

import (
	"fmt"
	"log"
	"os"
	"os/signal"
	"syscall"
)

//StartModule can be used to start the module
func StartModule(name string, handler func(*Buffer, chan bool)) {
	// Setup log module
	log.SetPrefix(fmt.Sprintf("[%s] ", name))

	sigc := make(chan os.Signal, 1)
	signal.Notify(sigc,
		syscall.SIGHUP,
		syscall.SIGINT,
		syscall.SIGTERM,
		syscall.SIGQUIT)

	buf := NewBuffer()

	go buf.Listen()

	quit := make(chan bool)

	go func() {
		sig := <-sigc
		log.Printf("Closing requests module (%v)", sig)
		quit <- true
	}()

	go handler(buf, quit)
	<-quit

}

//WriteResponse can be used to write a response
func WriteResponse(pid uint16, v interface{}) {
	data, err := PkgPack(pid, ProtoModuleRes, &v)
	if err == nil {
		_, err := os.Stdout.Write(data)
		if err != nil {
			log.Printf("Error writing to stdout: %s", err)
		}
	} else {
		log.Printf("Error creating package from value: %v", v)
	}
}

//WriteEx can be used to write an error response
func WriteEx(pid uint16, errCode Ex, errMsg string) {
	var errArr [2]interface{}

	errArr[0] = errCode
	errArr[1] = errMsg

	data, err := PkgPack(pid, ProtoModuleErr, &errArr)
	if err == nil {
		_, err := os.Stdout.Write(data)
		if err != nil {
			log.Printf("Error writing to stdout: %s", err)
		}
	} else {
		log.Printf("Error creating package from value: %v", errArr)
	}
}
