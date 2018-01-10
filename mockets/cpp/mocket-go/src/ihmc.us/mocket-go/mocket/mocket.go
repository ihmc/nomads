package mocket
//#cgo LDFLAGS: -L${SRCDIR}/../wrapper/ ${SRCDIR}/../wrapper/libmocketgowrapper.so -ldl
//#include "../wrapper/MocketWrapper.h"
//#include "../wrapper/ServerMocketWrapper.h"
import "C"
import "unsafe"
import (
	"C"
	"errors"
	"bytes"
	"encoding/binary"
	"log"
	"os"
	"net"
)

type Mocket struct {
	mocket C.CMocket
}

type ServerMocket struct {
	serverMocket C.CServerMocket
}

func NewMocket(configFile string) Mocket {
	var ret Mocket
	ret.mocket = C.MocketInit(C.CString(configFile))
	return ret
}

func NewMocketDtls(configFile string, certificateFile string, privateKey string) (Mocket, error) {
	var ret Mocket
	f, err := os.Open(certificateFile)
	if err != nil {
		log.Println("Certificate file: ", certificateFile, " not found")
		return Mocket{}, err
	}
	f.Close()
	_, err = os.Open(privateKey)
	if err != nil {
		log.Println("Private Key file: ", privateKey, " not found")
		return Mocket{}, err
	}
	ret.mocket = C.MocketInitDtls(C.CString(configFile), C.CString(certificateFile), C.CString(privateKey))
	return ret, nil
}

func (m Mocket) Close() (error) {
	var ret int 
	ret = int(C.MocketClose(m.mocket))
	if ret < 0 {
		return errors.New("Error during Close()")
	}
	return nil
}

func (m Mocket) Connect(remoteHost string, remotePort int) (error) {
	var ret int
	ret = int(C.MocketConnectNative(m.mocket, C.CString(remoteHost), C.int(remotePort)))
	if (ret < 0) {
		return errors.New("Error during Connect")
	}
	return nil
}

func (m Mocket) Send(reliable bool, sequenced bool, buffer []byte, ui32BufSize uint32, ui16Tag uint16, ui8Priority uint8,
ui32EnqueueTimeout uint32, ui32RetryTimeout uint32) (error) {
	var ret int
	var ireliable int
	var isequenced int
	if reliable {
		ireliable = 1
	} else {
		ireliable = 0
	}
	if sequenced {
		isequenced = 1
	} else {
		isequenced = 0
	}
	ret = int(C.MocketSend(m.mocket, C.int8_t(ireliable), C.int8_t(isequenced), (unsafe.Pointer(&buffer[0])), C.uint32_t(ui32BufSize), C.uint16_t(ui16Tag), C.uint8_t(ui8Priority), C.uint32_t(ui32EnqueueTimeout), C.uint32_t(ui32RetryTimeout)))
	if (ret < 0) {
		return errors.New("Error during Send")
	}
	return nil
}

func (m Mocket) SendBlob(buf []byte) (error){
	err := m.Send(true, true, buf, uint32(len(buf)), 0, uint8(5), 0, 0)
	return err
}

func (m Mocket) ReceiveBlob(bufsize uint32) (error) {
	buf := make([]byte, bufsize)
	err := m.Receive(buf, bufsize, 0)
	if err != nil {
		log.Println("Error in receiveBlob() ", err)
	}
	return err
}


func (m Mocket) Write32(ui32val uint32) (error){
	buf := make([]byte, 4)
	binary.BigEndian.PutUint32(buf, ui32val)
	err := m.Send(true, true, buf, uint32(len(buf)), 0, uint8(5), 0, 0)
	if err != nil {
		log.Println("Error in Write32()")
	}
	return err
}

func (m Mocket) Write16(ui16val uint16) (error) {
	buf := make([]byte, 2)
	binary.BigEndian.PutUint16(buf, ui16val)
	err := m.Send(true, true, buf, uint32(len(buf)), 0, uint8(5), 0, 0)
	if err != nil {
		log.Println("Error in Write16()")
	}
	return err
}

/**This function might contain a bug
 * Do not pass Go pointer to Go memory to cgo function
 */
func (m Mocket) Write8(ui8val uint8) (error) {
	buf := new(bytes.Buffer)
	binary.Write(buf, binary.BigEndian, &ui8val)
	err := m.Send(true, true, buf.Bytes(), uint32(len(buf.Bytes())), 0, uint8(5), 0, 0)
	if err != nil {
		log.Println("Error in Write16()")
	}
	return err
}

//    int MocketReceive (CMocket pmocket, void *pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);
func (m Mocket) Receive(buffer []byte, bufferSize uint32, timeout int64) (error) {
	var ret int
	ret = int(C.MocketReceive(m.mocket, unsafe.Pointer((&buffer[0])), C.uint32_t(bufferSize), C.int64_t(timeout)))
	if (ret < 0) {
		return errors.New("Error in receiving")
	}
	return nil
}

func (m Mocket) Read32() (uint32, error) {
	buf := make([]byte, 4)
	err := m.Receive(buf, 4, 0)
	if err != nil {
		return 0, err
	}
	i32ret := binary.BigEndian.Uint32(buf)
	return i32ret, nil
}

func (m Mocket) Read16() (uint16, error) {
	buf := make([]byte, 2)
	err := m.Receive(buf, 2, 0)
	if err != nil {
		return 0, err
	}
	i16ret := binary.BigEndian.Uint16(buf)
	return i16ret, nil
}

/**
 * TO-DO check if the conversion is correct
 */
func (m Mocket) Read8() (uint8, error) {
	buf := new(bytes.Buffer)
	rbuf := make([]byte, 1)
	err := m.Receive(rbuf, 1, 0)
	if err != nil {
		return 0, err
	}
	buf.Read(rbuf)
	binary.Write(buf, binary.LittleEndian, rbuf)
	ui8ret, err := buf.ReadByte()
	if err != nil {
		log.Println("Conversion error in read8()")
		return 0, nil
	}
	return uint8(ui8ret), nil
}


func (m Mocket) SetIdentifier(identifier string)  {
	C.MocketSetIdentifier(m.mocket, C.CString(identifier))
}

func (m Mocket) GetIdentifier() string {
	ret := C.GoString(C.MocketGetIdentifier(m.mocket))
	return ret
}

func (m Mocket) GetRemoteAddress() uint32 {
	return uint32(C.MocketGetRemoteAddress(m.mocket))
}

func (m Mocket) GetRemoteAddressString() string {
	ip := make(net.IP, 4)
	ui32addr := uint32(C.MocketGetRemoteAddress(m.mocket))
	binary.BigEndian.PutUint32(ip, ui32addr)
	return ip.String()
}

func (m Mocket) GetRemotePort() uint16 {
	return uint16(C.MocketGetRemotePort(m.mocket))
}

func (m Mocket) GetLocalAddress() uint32 {
	return uint32(C.MocketGetLocalAddress(m.mocket))
}

func (m Mocket) GetLocalPort() uint16 {
	return uint16(C.MocketGetLocalPort(m.mocket))
}


/**
 * ServerMocket functions
 */

func NewServerMocket(configFile string) ServerMocket {
	var ret ServerMocket
	ret.serverMocket = C.ServerMocketInit(C.CString(configFile))
	return ret
}

/**
 * This function creates a MocketDTLS
 */
func NewServerMocketDtls(configFile string, certificateFile string, privateKey string) (ServerMocket, error) {
	var ret ServerMocket
	f, err := os.Open(certificateFile)
	if err != nil {
		log.Println("Certificate file: ", certificateFile, " not found")
		return ServerMocket{}, err
	}
	f.Close()
	_, err = os.Open(privateKey)
	if err != nil {
		log.Println("Private Key file: ", privateKey, " not found")
		return ServerMocket{}, err
	}
	ret.serverMocket = C.ServerMocketInitDtls(C.CString(configFile), C.CString(certificateFile), C.CString(privateKey))
	return ret, nil
}

func (m ServerMocket) Close() (error) {
	var ret int
	ret = int(C.ServerMocketClose(m.serverMocket))
	if ret < 0 {
		return errors.New("Error during Close()")
	}
	return nil
}

func (m ServerMocket) Listen(ui16Port uint16) (error){
	var ret int
	ret = int(C.ServerMocketListen(m.serverMocket, C.uint16_t(ui16Port)))
	if ret < 0 {
		return errors.New("Listen, error in listening")
	}
	return nil
}

func (m ServerMocket) ListenAddress(ui16Port uint16, listenAddress string) (error){
	var ret int
	ret = int(C.ServerMocketListenAddress(m.serverMocket, C.uint16_t(ui16Port), C.CString(listenAddress)))
	if ret < 0 {
		return errors.New("ListenAddress, error in listening")
	}
	return nil
}

func (m ServerMocket) Accept() (Mocket) {
	var ret Mocket
	ret.mocket = C.ServerMocketAccept(m.serverMocket)
	return ret
}

func (m ServerMocket) AcceptPort(ui16PortForNewConnection uint16) (Mocket){
	var ret Mocket
	ret.mocket = C.ServerMocketAcceptPort(m.serverMocket, C.uint16_t(ui16PortForNewConnection))
	return ret
}



//uffer []byte, ui32BufSize uint32, ui16Tag uint16, ui8Priority uint8,
//ui32EnqueueTimeout uint32, ui32RetryTimeout uint32

