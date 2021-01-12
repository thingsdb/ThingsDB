package main

import (
	"log"
	tiext "requests/tiext"
)

func handler(buf *tiext.Buffer, done chan bool) {
	for {
		select {
		case pkg := <-buf.PkgCh:
			switch tiext.Proto(pkg.Tp) {
			case tiext.ProtoExtInit:
				log.Println("Initializing requests extension")
			default:
				log.Printf("Error: Unexpected package type: %d", pkg.Tp)
			}
			done <- false
			return

		case err := <-buf.ErrCh:
			log.Printf("Error: %s", err)
			done <- false
			return
		}
	}
}

func main() {
	buf := tiext.NewBuffer()

	go buf.Listen()

	done := make(chan bool)

	go handler(buf, done)
	<-done
}

// 	newBuffer()

// 	ch := make(chan string)
// 	go func(ch chan string) {
// 		bufio.NewReader()
// 		reader := bufio.NewReader(os.Stdin)

// 		for {
// 			// try to read the data
// 			wbuf := make([]byte, 8192)
// 			n, err := reader.Read(wbuf)

// 			if err != nil {
// 				// send an error if it's encountered
// 				close(ch)
// 				return
// 			}

// 			buf.len += uint32(n)
// 			buf.data = append(buf.data, wbuf[:n]...)

// 			for buf.len >= pkgHeaderSize {
// 				if buf.pkg == nil {
// 					buf.pkg, err = newPkg(buf.data)
// 					if err != nil {
// 						buf.errCh <- err
// 						return
// 					}
// 				}

// 				total := buf.pkg.size + pkgHeaderSize

// 				if buf.len < total {
// 					break
// 				}

// 				buf.pkg.setData(&buf.data, total)

// 				// The reserved ThingsDB events range is between 0..15
// 				if buf.pkg.tp >= 0 && buf.pkg.tp <= 15 {
// 					buf.evCh <- buf.pkg
// 				} else {
// 					buf.pkgCh <- buf.pkg
// 				}

// 				buf.data = buf.data[total:]
// 				buf.len -= total
// 				buf.pkg = nil
// 			}
// 		}

// 		for {
// 			s, err := reader.Read(wbuf)
// 			if err != nil {
// 				close(ch)
// 				return
// 			}

// 			ch <- s
// 		}
// 	}(ch)
// 	close(ch)

// stdinloop:
// 	for {
// 		select {
// 		case stdin, ok := <-ch:
// 			if !ok {
// 				break stdinloop
// 			} else {
// 				fmt.Println("Read input from stdin:", stdin)
// 			}
// 		case <-time.After(1 * time.Second):
// 			// Do something when there is nothing read from stdin
// 		}
// 	}
// 	fmt.Println("Done, stdin must be closed")
