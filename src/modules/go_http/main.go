package main

import (
	"log"
	f "gofrozen"
)

func http_init(backend uintptr) int{
	return 0
}
func http_configure(backend uintptr, config uintptr) int{
	return 0
}
func http_destroy(backend uintptr) int{
	return 0
}

func http_count(backend uintptr, hash uintptr) int{
	log.Print("go http count")
	n := f.Hash([]f.Hskel {
		f.Hitem(f.HK_backend, f.TYPE_STRINGT, "mooz" ),
		f.Hnext(hash) });

	return f.Backend_pass(backend, n)
}

func main(){
	g := f.NewBackend_t()
	g.SetClass("go_http")
	g.SetFunc_init(http_init)
	g.SetFunc_configure(http_configure)
	g.SetFunc_destroy(http_destroy)
	g.GetBackend_type_crwd().SetFunc_count(http_count)
	f.Class_register(g.Swigcptr())

	/*
	config  := f.Configs_string_parse(` { class="go_http" } `)
	backend := f.Backend_new(config)

	h := f.Hash([]f.Hskel {
		f.Hitem(f.HK_action, f.TYPE_UINT32T, f.ACTION_CRWD_COUNT ),
		f.Hend() });

	f.Backend_query(backend, h)
	*/
	return
}
