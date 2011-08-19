package main

import (
	"io"
	"log"
	"template"
	f "gofrozen"
)

// HK(data_key) HK(out_key) HK(go_template)

type templateExec_userdata struct {
	out_key                uint64
	data_key               uint64
}

func templateExec_configure(backend uintptr, config uintptr) int{
	data_key, ok := f.Hget(config, f.HK_data_key).(string)
	if !ok {
		log.Print("template_configure: no 'data_key' supplied")
		return -1
	}
	out_key, ok := f.Hget(config, f.HK_out_key).(string)
	if !ok {
		log.Print("template_configure: no 'out_key' supplied")
		return -1
	}

	userdata := &templateExec_userdata{
		data_key: f.Hash_string_to_key(data_key),
		out_key:  f.Hash_string_to_key(out_key) }
	f.Backend_SetUserdata(backend, userdata)
	return 0
}

func templateLoad_handler(backend uintptr, request uintptr) int {
	hash := f.Hash_find(request, f.HK_go_template)
	if hash == 0 {
		return -1
	}
	data := f.Hash_item_data(hash)

	tpl, e := template.Parse(" mooo ", nil)
	if e != nil {
		return -1
	}
	f.Data_assign(data, f.TYPE_GOINTERFACET, f.ObjToPtr(tpl), 0)
	return 0
}
func templateExecute_handler(backend uintptr, request uintptr) int {
	userdata := f.Backend_GetUserdata(backend).(*templateExec_userdata)

	h := f.Hash([]f.Hskel {
		f.Hitem(f.HK_go_template, f.TYPE_GOINTERFACET, nil ),
		f.Hnext(request) });
	if e := f.Backend_pass(backend, h); e < 0 {
		return e
	}

	tpl,  ok1 := f.Hget(h,        f.HK_go_template).(*template.Template)
	wr,   ok2 := f.Hget(request,  userdata.out_key).(io.Writer)
	data      := f.Hget(request,  userdata.data_key)
	if !ok1 || !ok2 {
		return -1
	}

	if e := tpl.Execute(data, wr); e != nil {
		return -1
	}
	return 0
}
func template_handler(backend uintptr, request uintptr) int {
	return 0
}

// go templateExec pass to underlying backend variable HK(go_template) in order to get complied template
// go_templateLoad reads underlying backend and treats input as template, then store it in hash and return
// go_template includes go_templateLoad and go_templateExec
func main(){
	gt := f.NewBackend_t()
	gt.SetClass("go_template")
	gt.SetSupported_api(f.API_HASH)
	gt.SetFunc_configure(templateExec_configure)
	gt.GetBackend_type_hash().SetFunc_handler(template_handler)
	f.Class_register(gt.Swigcptr())

	gl := f.NewBackend_t()
	gl.SetClass("go_templateLoad")
	gl.SetSupported_api(f.API_HASH)
	gl.GetBackend_type_hash().SetFunc_handler(templateLoad_handler)
	f.Class_register(gl.Swigcptr())

	ge := f.NewBackend_t()
	ge.SetClass("go_templateExec")
	ge.SetSupported_api(f.API_HASH)
	ge.SetFunc_configure(templateExec_configure)
	ge.GetBackend_type_hash().SetFunc_handler(templateExecute_handler)
	f.Class_register(ge.Swigcptr())
	return
}

