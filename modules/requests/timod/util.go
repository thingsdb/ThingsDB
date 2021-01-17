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
