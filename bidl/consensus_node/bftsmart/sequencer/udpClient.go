package main

import (
	"bytes"
	"flag"
	"fmt"

	// "time"
	"bufio"
	"encoding/binary"
	"io/ioutil"
	"net"
	"os"
)

var host = flag.String("host", "localhost", "host")
var port = flag.String("port", "8888", "port")
var tps = flag.Int("tps", 1000, "tps")
var number = flag.Int("num", 500, "number of transcations")
var filepath = flag.String("file", "", "filepath, if empty, then read from stdin")

func main() {
	flag.Parse()
	addr, err := net.ResolveUDPAddr("udp", *host+":"+*port)
	if err != nil {
		fmt.Println("Can't resolve address: ", err)
		os.Exit(1)
	}
	conn, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		fmt.Println("Can't dial: ", err)
		os.Exit(1)
	}
	defer conn.Close()
	var msg []byte
	if *filepath == "" {
		// read from stdin
		fmt.Println("Please input your msg: ")
		input := bufio.NewScanner(os.Stdin)
		input.Scan()
		temp := input.Text()
		msg = []byte(temp)

	} else {
		// read from file
		msg, _ = ioutil.ReadFile(*filepath)
	}

	var seq int64 = 0

	for i := 0; i < *number; i += 1 {
		// time.Sleep(time.Duration(int(1000000/(*tps)))*time.Microsecond)
		bytebuf := bytes.NewBuffer([]byte{})
		binary.Write(bytebuf, binary.BigEndian, seq)
		// fmt.Println(bytebuf.Bytes())
		seq = seq + 1
		_, err = conn.Write(append(bytebuf.Bytes(), msg...))
		if err != nil {
			fmt.Println("failed:", err)
			os.Exit(1)
		}
	}

	// data := make([]byte, 1000)
	// _, err = conn.Read(data)
	// if err != nil {
	//         fmt.Println("failed to read UDP msg because of ", err)
	//         os.Exit(1)
	// }
	// fmt.Println(string(data))
	os.Exit(0)
}
