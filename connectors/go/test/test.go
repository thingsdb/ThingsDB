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

	// if err := conn.AuthToken("aoaOPzCZ1y+/f0S/jL1DUB"); err != nil {
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
	// conf := &tls.Config{
	// 	InsecureSkipVerify: true,
	// }
	// conn := client.NewConn("35.204.223.30", 9400, conf)
	conn := client.NewConn("localhost", 9200, nil)
	ok := make(chan bool)

	go example(conn, ok)

	<-ok
}
