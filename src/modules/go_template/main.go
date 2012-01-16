package main

import (
	"io"
	"log"
	"fmt"
	"template"
	f "gofrozen"
)

// HK(error) HK(input) HK(output) HK(go_template)

type templateExec_userdata struct {
	k_output               f.Enum_SS_hashkey_t
	k_input                f.Enum_SS_hashkey_t
}
	func templateExec_configure(machine uintptr, config uintptr) int{
		d_input, ok := f.Hget(config, f.HK_input).(f.Enum_SS_hashkey_t)
		if !ok {
			log.Print("template_configure: no HK(input) supplied")
			return -1
		}

		d_output, ok := f.Hget(config, f.HK_output).(f.Enum_SS_hashkey_t)
		if !ok {
			log.Print("template_configure: no HK(output) supplied")
			return -1
		}

		userdata := &templateExec_userdata{
			k_input:   d_input,
			k_output:  d_output }
		f.Machine_SetUserdata(machine, userdata)
		return 0
	}

func template_load(machine uintptr, request uintptr) (tpl *template.Template, err string) {
	var tpl_str []byte

	// request size
	q_count := f.Hash([]f.Hskel {
		f.Hitem(f.HK_action, f.TYPE_UINT32T, f.ACTION_COUNT),
		f.Hitem(f.HK_buffer, f.TYPE_SIZET,   0),
		f.Hnext(request) })
	f.Machine_pass(machine, q_count)

	str_len, ok := f.Hget(q_count, f.HK_buffer).(uint)
	if !ok || str_len == 0 {
		tpl_str = []byte("")
	}else{
		tpl_str = make([]byte, str_len)

		// read all
		q_read := f.Hash([]f.Hskel {
			f.Hitem(f.HK_action, f.TYPE_UINT32T, f.ACTION_READ),
			f.Hitem(f.HK_buffer, f.TYPE_RAWT,    tpl_str),
			f.Hnext(request) })

		f.Machine_pass(machine, q_read)
	}

	tpl, e := template.Parse(string(tpl_str), nil) // TODO BAD BAD BAD
	if e != nil {
		err := fmt.Sprintf("templateLoad_handler template parse error: %v", e)
		return nil, err
	}
	return tpl, ""
}

func template_exec(tpl *template.Template, request uintptr, k_output f.Enum_SS_hashkey_t, k_input f.Enum_SS_hashkey_t)(err string) {
	wr,   ok2 := f.Hget(request,  k_output).(io.Writer)
	data      := f.Hget(request,  k_input)
	if !ok2 {
		return fmt.Sprintf("templateExecute_handler input data invalid: tpl {%v}; output {%v, %v}; data {%v}",
			tpl, wr, ok2,  data)
	}

	if e := tpl.Execute(data, wr); e != nil {
		return fmt.Sprintf("templateExecute_handler template execution error: %v", e)
	}
	return ""
}

func templateLoad_handler(machine uintptr, request uintptr) int {
	hash := f.Hash_find(request, f.HK_go_template)
	if hash == 0 {
		log.Printf("templateLoad_handler HK(go_template) not supplied")
		return -1
	}
	data := f.Hash_item_data(hash)

	tpl, err := template_load(machine, request)
	if err != "" {
		log.Printf(err)
		return -1
	}

	f.Data_assign(data, f.TYPE_GOINTERFACET, f.ObjToPtr(tpl))
	return 0
}

func templateExecute_handler(machine uintptr, request uintptr) int {
	userdata := f.Machine_GetUserdata(machine).(*templateExec_userdata)

	h := f.Hash([]f.Hskel {
		f.Hitem(f.HK_go_template, f.TYPE_GOINTERFACET, nil ),
		f.Hnext(request) });
	if e := f.Machine_pass(machine, h); e < 0 {
		return e
	}
	tpl,  ok1 := f.Hget(h,        f.HK_go_template).(*template.Template)
	if !ok1 {
		log.Print("templateExecute_handler HK(go_template) not supplied")
		return -1
	}

	err := template_exec(tpl, request, userdata.k_output, userdata.k_input)
	if err != "" {
		log.Printf(err)
		return -1
	}

	return 0
}

func template_handler(machine uintptr, request uintptr) int {
	userdata := f.Machine_GetUserdata(machine).(*templateExec_userdata)

	tpl, err := template_load(machine, request)
	if err != "" {
		log.Printf(err)
		return -1
	}

	if err := template_exec(tpl, request, userdata.k_output, userdata.k_input); err != "" {
		log.Printf(err)
		return -1
	}

	return 0
}

// go templateExec pass to underlying machine variable HK(go_template) in order to get complied template
// go_templateLoad reads underlying machine and treats input as template, then data it in hash and return
// go_template includes go_templateLoad and go_templateExec
func main(){
	gt := f.NewMachine_t()
	gt.SetClass("go_template")
	gt.SetSupported_api(f.API_HASH)
	gt.SetFunc_configure(templateExec_configure)
	gt.GetMachine_type_hash().SetFunc_handler(template_handler)
	f.Class_register(gt.Swigcptr())

	gl := f.NewMachine_t()
	gl.SetClass("go_templateLoad")
	gl.SetSupported_api(f.API_HASH)
	gl.GetMachine_type_hash().SetFunc_handler(templateLoad_handler)
	f.Class_register(gl.Swigcptr())

	ge := f.NewMachine_t()
	ge.SetClass("go_templateExec")
	ge.SetSupported_api(f.API_HASH)
	ge.SetFunc_configure(templateExec_configure)
	ge.GetMachine_type_hash().SetFunc_handler(templateExecute_handler)
	f.Class_register(ge.Swigcptr())
	return
}

