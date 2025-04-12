FROM golang:latest

WORKDIR /apps

COPY go.mod go.sum ./

RUN go mod tidy

COPY . .

RUN go build -o server

EXPOSE 22222

CMD ["/apps/server"]
