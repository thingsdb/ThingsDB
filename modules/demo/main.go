/*
 * Demo is a ThingsDB module which may be used as a template to build modules.
 *
 * This module simply extract a given `message` property from a request and
 * returns this message.
 *
 * For example:
 *
 *     // Create the module (@thingsdb scope)
 *     new_module('DEMO', 'demo', nil, nil);
 *
 *     // When the module is loaded, use the module in a future
 *     future({
 *       module: 'DEMO',
 *       message: 'Hi ThingsDB module!',
 *     }).then(|msg| {
 *	      `Got the message back: {msg}`
 *     });
 */
package main

import (
	"fmt"
	"log"

	timod "github.com/thingsdb/go-timod"

	"github.com/vmihailenco/msgpack"
)

func handler(buf *timod.Buffer, quit chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch timod.Proto(pkg.Tp) {
			case timod.ProtoModuleConf:
				/*
				 * Configuration data for this module is received from ThingsDB.
				 * The module should respond with `timod.WriteConfOk()` if successful
				 * or `timod.WriteConfErr() if the module could not be correctly
				 * configured.
				 */
				log.Println("No configuration data is required for this module")
				timod.WriteConfOk() // Just write OK

			case timod.ProtoModuleReq:
				/*
				 * A request from ThingsDB may be unpacked to a struct or to
				 * an map[string]interface{}.
				 *
				 * The module should respond with `
				 */
				type Demo struct {
					Message string `msgpack:"message"`
				}
				var demo Demo

				err := msgpack.Unmarshal(pkg.Data, &demo)
				if err == nil {
					/*
					 * In this demo a `message` property will be unpacked and
					 * used as a return value.
					 */
					timod.WriteResponse(pkg.Pid, &demo.Message)
				} else {
					/*
					 * In case of an error, make sure to call `WriteEx(..)` so ThingsDB
					 * can finish the future request with an appropiate error.
					 */
					timod.WriteEx(pkg.Pid, timod.ExBadData, fmt.Sprintf("failed to unpack request (%s)", err))
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
	// Starts the module
	timod.StartModule("demo", handler)
}
