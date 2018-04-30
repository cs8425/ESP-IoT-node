package main

import (
	"net/http"
	"path/filepath"
	"os"
	"flag"
	"log"
)

var verbosity = flag.Int("v", 3, "verbosity")
var port = flag.String("l", ":8080", "bind port")
var dir = flag.String("d", "./data/web", "bind dir")

func reqlog(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		Vln(3, "[  ]", r.Method, r.URL, r.RemoteAddr)
		next.ServeHTTP(w, r)
	})
}

func trygz(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		gzpath := filepath.Join(*dir, r.URL.Path + ".gz")
		_, err := os.Stat(gzpath)
		if err == nil {
			r.URL.Path += ".gz"
			w.Header().Set("Content-Encoding", "gzip")
			Vln(3, "[gz]", r.Method, r.URL)
		}
		next.ServeHTTP(w, r)
	})
}

func main() {
	flag.Parse()
	http.Handle("/", trygz(reqlog(http.FileServer(http.Dir(*dir)))))
	err := http.ListenAndServe(*port, nil)
	if err != nil {
		Vln(0, err)
	}
}

func Vf(level int, format string, v ...interface{}) {
	if level <= *verbosity {
		log.Printf(format, v...)
	}
}
func V(level int, v ...interface{}) {
	if level <= *verbosity {
		log.Print(v...)
	}
}
func Vln(level int, v ...interface{}) {
	if level <= *verbosity {
		log.Println(v...)
	}
}

