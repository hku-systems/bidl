package chaincode

import (
	"fmt"
	"math/rand"
	"time"
)


var chs = []rune("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890!@#$%^&*()_=")


func getName(n int) string {
	b := make([]rune, n)
	for i := range b {
		b[i] = chs[rand.Intn(len(chs))]
	}
	return string(b)
}

func generate() string {
	idx := getName(64)
	res := fmt.Sprintf("{\"Args\":[\"create_account\", \"%s\", \"%s\", \"1000000\", \"1000000\"]}", idx, idx)
	return res
}

func GenerateWorkload(n int) []string {
	rand.Seed(time.Now().UnixNano())
	i := 0
	var res []string
	for i = 0; i < n; i++ {
		res = append(res, generate())
	}
	return res
}
