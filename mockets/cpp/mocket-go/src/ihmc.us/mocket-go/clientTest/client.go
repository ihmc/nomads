package main

import "C"
import (
	"fmt"
	"ihmc.us/mocket-go/mocket"
	"log"
)

func main() {
	m, err := mocket.NewMocketDtls("mockets.conf", "client.pem", "client-key.pem")
	if err != nil {
		log.Println("Error ", err)
		return
	}
	err = m.Connect("localhost", 50001)
	if err != nil {
		log.Println("Error ", err)
		return
	}
	log.Println("Sending: " + "Hello MocketDTLS")
	m.Send(false, false, []byte("Hello MocketDTLS"), uint32(len([]byte("Hello MocketDTLS"))), 0, 0, 0, 0)
	fmt.Println("Closing Mocket client")
	m.Close()
}
