package main

import (
	"os"
	"net"
	"log"
	"http"
	f "gofrozen"
)

// HK(addr) HK(url) HK(go_req)    // dont remove

type myHttp struct {
	backend uintptr
}
	func (srv *myHttp) Run(addr string) os.Error {
		mux := http.NewServeMux()
		mux.HandleFunc("/", func (w http.ResponseWriter, req *http.Request){
			h := f.Hash([]f.Hskel {
				f.Hitem(f.HK_action, f.TYPE_UINT32T,      f.ACTION_CRWD_CREATE ),
				f.Hitem(f.HK_url,    f.TYPE_STRINGT,      req.URL.RawPath ),
				f.Hitem(f.HK_go_req, f.TYPE_GOINTERFACET, req),
				f.Hend() });

			f.Backend_query(srv.backend, h)
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

	srv := &myHttp{ backend: backend }
	if e := srv.Run(addr); e != nil {
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

