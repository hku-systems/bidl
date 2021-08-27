package main

import (
	"normal_node/cmd/common"
)

type Server struct {
	messages chan common.Message
	BlkSize int
	ID      int
	tput    int
}

func NewServer(blkSize int, id int, tput int) *Server {
	return &Server{
		messages: make(chan common.Message),
		BlkSize:  blkSize,
		ID:       id,
		tput:     tput,
	}
}
