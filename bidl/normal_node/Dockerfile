FROM golang:1.16-alpine
COPY . /normal_node
WORKDIR /normal_node

RUN go build -o server ./cmd/server
RUN go build -o client ./cmd/client

RUN chmod +x server
RUN chmod +x client
