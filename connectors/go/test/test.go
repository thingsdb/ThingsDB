package main

import (
	"fmt"

	"../client"
)

type TIface interface {
}

type TModel struct {
}

type Label struct {
	name        string
	description string
}

type OsData struct {
	labels []Label
}

// var oversight Collection

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

	if err := conn.Authenticate("admin", "pass"); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	reqThingsDB := client.NewReqThingsDB("users();")

	if res, err = conn.Query(reqThingsDB); err != nil {
		println(err.Error())
		ok <- false
		return
	}

	fmt.Printf("%v\n", res)

	reqCollection := client.NewReqCollection("map(|k|k);", "stuff", 1)
	if res, err = conn.Query(reqCollection); err != nil {
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
