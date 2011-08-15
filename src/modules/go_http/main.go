package main

import (
	"io"
	"os"
	//"net"
	"log"
	"http"
	f "gofrozen"
)

// HK(addr) HK(url)     // dont remove

type myHttp struct {
	backend uintptr
}
	func (srv *myHttp) Run(addr string) os.Error {
		mux := http.NewServeMux()
		//mux.HandleFunc("/", func (w http.ResponseWriter, req *http.Request){
		//	h := f.Hash([]f.Hskel {
		//		f.Hitem(f.HK_url, f.TYPE_STRINGT, req.URL.RawPath ),
		//		f.Hend() });
		//
		//	f.Backend_query(srv.backend, h)
		//})
		mux.HandleFunc("/", handleFunc)

		//l, e := net.Listen("tcp", addr)
		//if e != nil {
		//	return e
		//}
		//go http.Serve(l, mux)
		err := http.ListenAndServe(addr, mux)
		if err != nil {
			log.Print(err)
		}
		return nil
	}

	func handleFunc(w http.ResponseWriter, req *http.Request){
		io.WriteString(w, "Hello")
		log.Print("Hello")
	}

func http_init(backend uintptr) int{
	return 0
}
func http_configure(backend uintptr, config uintptr) int{
	addr, ok := f.Hget(config, f.HK_addr).(string)
	log.Printf("addr %v", []byte(addr))

	srv := &myHttp{ backend: backend }
	if e := srv.Run(addr); e != nil {
		log.Print("Failed %v", e)
		return -1
	}
	log.Print("Ok")
	return 0
}
func http_destroy(backend uintptr) int{
	return 0
}

func main(){
	g := f.NewBackend_t()
	g.SetClass("go_http")
	g.SetFunc_init(http_init)
	g.SetFunc_configure(http_configure)
	g.SetFunc_destroy(http_destroy)
	f.Class_register(g.Swigcptr())
	
	//srv := &myHttp{ }
	//if e := srv.Run(":12345"); e != nil {
	//	log.Print("Failed %v", e)
	//	return
	//}
	//log.Print("Ok")
	
	return
}

/*
	g.GetBackend_type_crwd().SetFunc_count(http_count)

func http_count(backend uintptr, hash uintptr) int{
	log.Print("go http count")
	n := f.Hash([]f.Hskel {
		f.Hitem(f.HK_backend, f.TYPE_STRINGT, "mooz" ),
		f.Hnext(hash) });

	return f.Backend_pass(backend, n)
}
	config  := f.Configs_string_parse(` { class="go_http" } `)
	backend := f.Backend_new(config)

	h := f.Hash([]f.Hskel {
		f.Hitem(f.HK_action, f.TYPE_UINT32T, f.ACTION_CRWD_COUNT ),
		f.Hend() });

	f.Backend_query(backend, h)
	*/
