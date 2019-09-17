package main

import (
	"fmt"

	"../client"
)

func example(conn *client.Conn, ok chan bool) {
	var res interface{}
	var err error

	if err := conn.Connect(); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	// oversight.Watch()

	defer conn.Close()

	if err := conn.AuthPassword("admin", "pass"); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	if res, err = conn.Query("@thingsdb", "users_info();", nil, 120); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	fmt.Printf("%v\n", res)

	ok <- true
}

func main() {
	conn := client.NewConn("127.0.0.1", 9200)

	ok := make(chan bool)

	go example(conn, ok)

	<-ok
}
