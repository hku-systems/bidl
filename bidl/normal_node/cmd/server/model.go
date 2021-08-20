package main

import (
	"normal_node/cmd/common"
)

type Server struct {
	messages chan common.Message
	BlkSize int
	ID      int
}

func NewServer(blkSize int, id int) *Server {
	return &Server{
		messages: make(chan common.Message),
		BlkSize:  blkSize,
		ID:       id,
	}
}
