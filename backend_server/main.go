package main

import (
	"fmt"
	"log"
	"net/http"
	"os"

	"github.com/joho/godotenv"
)

var SERVER_PORT string

func init() {
	godotenv.Load()
	SERVER_PORT = os.Getenv("BACKEND_PORT")
	if SERVER_PORT == "" {
		log.Fatal(".env not loaded.")
	}
}

func IndexHandler(rw http.ResponseWriter, r *http.Request) {
	log.Printf("Recieved request: %s\n", r.Header)
	fmt.Fprintf(rw, "Recieved request from server: %s", SERVER_PORT)
}

func main() {
	http.HandleFunc("/", IndexHandler)

	fmt.Printf("Server is listening on port: %s\n", SERVER_PORT)
	http.ListenAndServe(":"+SERVER_PORT, nil)
}
