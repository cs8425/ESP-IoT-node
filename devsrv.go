/*
   Simple http server for development and pack data to .gz
*/
package main

import (
	"bufio"
	"compress/gzip"
	"net/http"
	"path/filepath"
	"mime"
	"io"
	"io/ioutil"
	"strings"
	"os"
	"flag"
	"log"
)

var verbosity = flag.Int("v", 3, "verbosity")
var port = flag.String("l", ":8080", "bind port")
var dir = flag.String("d", "./data/web", "output dir")
var sdir = flag.String("s", "./web", "source dir")

func trygz(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fpath := r.URL.Path

		if strings.HasSuffix(r.URL.Path, "/") {
			fpath = fpath + "index.html"
		}

		gzpath := filepath.Join(*dir, fpath + ".gz")
		_, err := os.Stat(gzpath)
		if err == nil {
			ctype := mime.TypeByExtension(filepath.Ext(fpath))
			r.URL.Path = fpath + ".gz"
			w.Header().Set("Content-Type", ctype)
			w.Header().Set("Content-Encoding", "gzip")
			Vln(3, "[gz]", r.Method, fpath, "->", r.URL, r.RemoteAddr)
		} else {
			Vln(3, "[  ]", r.Method, r.URL, r.RemoteAddr)
		}
		next.ServeHTTP(w, r)
	})
}

func genGz(indir string, outdir string) {
	files, err := ioutil.ReadDir(indir)
	if err != nil {
		Vln(2, "[Err] read input dir err", indir, err)
		return
	}

	jslist := make([]string, 0)
	csslist := make([]string, 0)
	for _, file := range files {
		if file.IsDir() {
			continue
		}

		name := file.Name()
		fpath := filepath.Join(indir, name)

		switch {
		case strings.HasSuffix(name, ".js"):
			Vln(5, "[file]js", name, fpath)
			jslist = append(jslist, fpath)

		case strings.HasSuffix(name, ".css"):
			Vln(5, "[file]css", name, fpath)
			csslist = append(csslist, fpath)

		default:
			gzpath := filepath.Join(outdir, name + ".gz")
			Vln(5, "[file]", name, fpath, ">>", gzpath)
			fi, err := os.OpenFile(fpath, os.O_RDONLY, 0400)
			if err != nil {
				Vln(2, "[Err] gz input file err", fpath, err)
				return
			}

			gzFile(fi, gzpath)
		}

	}

	jspath := filepath.Join(outdir, "app.js.gz")
	gzFiles(jslist, jspath)

	csspath := filepath.Join(outdir, "app.css.gz")
	gzFiles(csslist, csspath)
}

func gzFiles(list []string, outpath string) {
	fds := make([]io.Reader, 0, len(list) * 2)
	for _, fpath := range list {
		Vln(5, "[gzFiles]", fpath, ">>", outpath)
		fi, err := os.OpenFile(fpath, os.O_RDONLY, 0400)
		if err != nil {
			Vln(2, "[Err] gz input file err", fpath, err)
			continue
		}

		fds = append(fds, fi, strings.NewReader("\n"))
	}

	r := io.MultiReader(fds...)
	gzFile(r, outpath)
}

func gzFile(fi io.Reader, outpath string) {
	fo, err := os.OpenFile(outpath, os.O_CREATE | os.O_TRUNC | os.O_WRONLY, 0644)
	if err != nil {
		Vln(2, "[Err] gz output file err", outpath, err)
		return
	}

	w, _ := gzip.NewWriterLevel(fo, gzip.BestCompression)
	defer w.Close()

	io.Copy(w, fi)
}

func main() {
	flag.Parse()

	genGz(*sdir, *dir)
	go regzip()

	http.Handle("/", trygz(http.FileServer(http.Dir(*dir))))
	err := http.ListenAndServe(*port, nil)
	if err != nil {
		Vln(0, err)
	}
}

func regzip() {
	Vln(1, "press enter for regenerate gzip files")

	reader := bufio.NewReader(os.Stdin)
	for {
		reader.ReadString('\n')
		Vln(1, "regenerate gzip...")
		genGz(*sdir, *dir)
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

