package main

import (
	"fmt"
	"ihmc.us/mocket-go/mocket"
	"log"
)

func main() {
	sm, err := mocket.NewServerMocketDtls("mockets.conf", "server.pem", "server-key.pem")
	if err != nil {
		log.Println("Error in Creating ServerMocketDtls", err)
		return
	}
	fmt.Println("Created Servermocket")
	err = sm.Listen(50001)
	if err != nil {
		log.Println("Error on listen, ", err)
		return
	}
	fmt.Println("Servermocket Listen on port 50001 ", err)
	for {
		m := sm.Accept()
		log.Println("New connection from: ", m.GetRemoteAddressString(), " Port: ", m.GetRemotePort())
		buf := make([]byte, 200)
		err = m.Receive(buf, 200, 150)
		if err != nil {
			log.Println("Error in receive: ", err)
		}
		log.Println("Received ", buf, string(buf))
		m.Close()
	}
	quit := make(chan bool, 1)
	<-quit
}
