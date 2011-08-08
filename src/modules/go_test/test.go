package main

import (
	"log"
	f "gofrozen"
)

func test_init(backend uintptr) int{
	log.Print("go test init")
	return 0
}
func test_count(backend uintptr, hash uintptr) int{
	log.Print("go test count")
	n := f.Hash([]f.Hskel {
		f.Hitem(f.HK_backend, f.TYPE_STRINGT, "mooz" ),
		f.Hnext(hash) });

	f.Backend_test_pass(backend, n)
	return 0
}

func main(){
	g := f.NewBackend_t()
	g.SetName("test_name")
	g.SetFunc_init(test_init)
	g.GetBackend_type_crwd().SetFunc_count(test_count)
	f.Backend_test(g.Swigcptr())
	return
}
