package main

import (
	"../client"
)

func example(conn *client.Conn, ok chan bool) {
	if err := conn.Connect(); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	defer conn.Close()

	if err := conn.Authenticate("admin", "pas"); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	ok <- true
}

func main() {
	conn := client.NewConn("127.0.0.1", 9200)

	ok := make(chan bool)

	go example(conn, ok)

	<-ok
}
