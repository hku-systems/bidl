package config

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

var Hosts map[int]string

func ReadHosts() {
	file, err := os.Open("./hosts.config")
	if err != nil {
		panic(err)
	}
	defer file.Close()
	Hosts := make(map[int]string)
	buf := bufio.NewReader(file)
	for {
		line, err := buf.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				fmt.Println("File read ok!")
				break
			} else {
				fmt.Println("Read file error!", err)
				return
			}
		}
		line = strings.TrimSpace(line)
		hostInfo := strings.Split(line, " ")
		id, _ := strconv.Atoi(hostInfo[0])
		addr := hostInfo[1]
		Hosts[id] = addr
	}
}
