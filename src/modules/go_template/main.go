package main

import (
	//"os"
	//"net"
	//"log"
	//"template"
	f "gofrozen"
)

func template_configure(backend uintptr, config uintptr) int{
	//addr, ok := f.Hget(config, f.HK_addr).(string)
	return 0
}

func main(){
	g := f.NewBackend_t()
	g.SetClass("go_template")
	g.SetSupported_api(f.API_HASH)
	g.SetFunc_configure(template_configure)
	f.Class_register(g.Swigcptr())
	return
}

