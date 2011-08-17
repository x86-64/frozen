package main

import (
	"os"
	"net"
	"log"
	"http"
	f "gofrozen"
)

// HK(addr) HK(url) HK(http_resp) HK(http_req)    // dont remove

func runServer(addr string, backend uintptr) os.Error {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func (resp http.ResponseWriter, req *http.Request){
		h := f.Hash([]f.Hskel {
			f.Hitem(f.HK_url,        f.TYPE_STRINGT,      req.URL.RawPath ),
			f.Hitem(f.HK_http_resp,  f.TYPE_GOINTERFACET, resp),
			f.Hitem(f.HK_http_req,   f.TYPE_GOINTERFACET, req),
			f.Hend() });

		f.Backend_query(backend, h)
	})

	l, e := net.Listen("tcp", addr)
	if e != nil {
		return e
	}
	go http.Serve(l, mux)
	return nil
}

func http_configure(backend uintptr, config uintptr) int{
	addr, ok := f.Hget(config, f.HK_addr).(string)

	if e := runServer(addr, backend); e != nil {
		log.Print("go_http failed configure: %v", e)
		return -1
	}
	return 0
}

func main(){
	g := f.NewBackend_t()
	g.SetClass("go_http")
	g.SetSupported_api(f.API_HASH)
	g.SetFunc_configure(http_configure)
	f.Class_register(g.Swigcptr())
	return
}

