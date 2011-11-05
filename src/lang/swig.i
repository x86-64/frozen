%module Frozen

#define API

%{
#include "libfrozen.h"
%}

#ifdef SWIGGO
#define DEBUG // for missing -DDEBUG from configure

%typemap(gotype) f_init,      const void * & "func(uintptr) int"
%typemap(gotype) f_fork,      const void * & "func(uintptr, uintptr, uintptr) int"
%typemap(gotype) f_configure, const void * & "func(uintptr, uintptr) int"
%typemap(gotype) f_destroy,   const void * & "func(uintptr) int"
%typemap(gotype) f_crwd,      const void * & "func(uintptr, uintptr) int"

%typemap(gotype) backend_t *, const void * & "uintptr"
%typemap(gotype) hash_t *,    const void * & "uintptr"
%typemap(gotype) request_t *, const void * & "uintptr"
#endif

%include hashkeys.i

%include ../../libfrozen/core/api.h
%include ../../libfrozen/core/data.h
%include ../../libfrozen/core/data_selected.h
%include ../../libfrozen/core/backend.h
%include ../../libfrozen/core/hash.h
%include ../../libfrozen/core/errors.h
%include ../../libfrozen/core/configs/config.h

typedef hash_t                   request_t;
typedef signed int               ssize_t;
typedef unsigned int             size_t;
typedef signed long long int     intmax_t;
typedef unsigned long long int   uintmax_t;
typedef uintmax_t                hash_key_t;

int                frozen_init(void);
int                frozen_destroy(void);

#ifdef SWIGPERL // {{{
// temprorary solution, need remove data_convert

%include cstring.i

%perlcode %{
INIT    { frozen_init();  }
DESTROY { frozen_destroy(); }

sub query {
	my $backend = shift;
	my $request = shift;
	my $code    = (shift or undef);
	
	my ($arrsz, $h_request, $ret);
	
	$arrsz = (scalar keys %$request) + 1;
	
	$h_request = hash_new($arrsz);
	while(my ($k,$va) = each(%$request)){
		my ($t,$v) = %$va;
		
		hash_set($h_request, $k, $t, "$v");
	}
	
	$ret = backend_query($backend, $h_request);
	
	if(defined $code){
		&$code($h_request);
	}
	
	hash_free($h_request);
	
	return $ret;
}
%}

#endif // }}}
#ifdef SWIGGO // {{{
#ifdef SWIGGO_GCCGO // {{{
%insert("go_wrapper") %{
func ObjToPtr(e interface {}) uintptr {
        rv := reflect.NewValue(&e)
        return rv.Addr()
}
func ObjFromPtr(ptr uintptr) interface {} {
        var z *interface {}
        rv := reflect.NewValue(z)
        sn := unsafe.Unreflect(rv.Type(), unsafe.Pointer(ptr)).(*interface {})
        return *sn
}
%}
#else
%insert("go_wrapper") %{
func ObjToPtr(e interface {}) uintptr {
        rv := reflect.ValueOf(&e)
        return rv.UnsafeAddr()
}
func ObjFromPtr(ptr uintptr) interface {} {
        var z *interface {}
        rv := reflect.ValueOf(z)
        sn := unsafe.Unreflect(rv.Type(), unsafe.Pointer(ptr)).(*interface {})
        return *sn
}
%}
#endif // }}}

%typemap(gotype) (char **string, size_t *string_len) "[]string"
%typemap(in) (char **string, size_t *string_len) {
	_gostring_ *a = (_gostring_ *)$input.array;
	$1 = ($1_ltype)&(a[0].p);
	$2 = ($2_ltype)&(a[0].n);
}

%inline %{

void __morestack(void){}

void go_data_to_raw(data_t *data, char **string, size_t *string_len){
	fastcall_getdataptr r_ptr = { { 3, ACTION_GETDATAPTR } };
	data_query(data, &r_ptr);
	
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	data_query(data, &r_len);
	
	*string     = r_ptr.ptr;
	*string_len = r_len.length;
}
uintmax_t  go_data_to_uint(data_t *data){
	uintmax_t ret = 0;
	memcpy(&ret, data->ptr, sizeof(ret));
	return ret;
}

void _backend_setuserdata(backend_t *backend, void *data){
	backend->userdata = data;
}
void *_backend_getuserdata(backend_t *backend){
	return backend->userdata;
}
void data_assign(data_t *data, datatype_t type, void *ptr){
	data->type = type;
	data->ptr  = ptr;
}

data_t * hash_item_data(hash_t *hash){
	return &hash->data;
}

%}

%insert("go_begin") %{
	import "fmt"
	import "unsafe"
	import "reflect"
%}

%insert("go_wrapper") %{

func Backend_SetUserdata(backend uintptr, userdata interface{}) {
	X_backend_setuserdata(backend, ObjToPtr(userdata))
}
func Backend_GetUserdata(backend uintptr) interface {} {
	return ObjFromPtr( X_backend_getuserdata(backend) )
}

type Hskel struct {
	Key           uint64
	Data_type     Enum_SS_datatype_t
	Data_ptr      unsafe.Pointer
}; 

type Traw_t struct {
	Ptr           unsafe.Pointer
	Size          uint
};

func hitem_getptr(s interface {}) unsafe.Pointer {
        var data_ptr  unsafe.Pointer
	
	switch v := s.(type) {
		case  int:   data_ptr = unsafe.Pointer(&v)
		case uint:   data_ptr = unsafe.Pointer(&v)
		case  int16: data_ptr = unsafe.Pointer(&v)
		case uint16: data_ptr = unsafe.Pointer(&v)
		case  int32: data_ptr = unsafe.Pointer(&v)
		case uint32: data_ptr = unsafe.Pointer(&v)
		case  int64: data_ptr = unsafe.Pointer(&v)
		case uint64: data_ptr = unsafe.Pointer(&v)
		case string: data_ptr = unsafe.Pointer(&([]byte( v )[0]))
		case []byte: data_ptr = unsafe.Pointer(&v[0])
		case Enum_SS_data_functions: 
			     data_ptr = unsafe.Pointer(&v)
		default: fmt.Printf("Hitem: unexpected type %T\n", v)
	}
	return data_ptr
}

func hitem_getlen(s interface {}) uint {
	switch v := s.(type) {
	       case  int:   return uint(unsafe.Sizeof(v))
	       case uint:   return uint(unsafe.Sizeof(v))
	       case  int16: return uint(unsafe.Sizeof(v))
	       case uint16: return uint(unsafe.Sizeof(v))
	       case  int32: return uint(unsafe.Sizeof(v))
	       case uint32: return uint(unsafe.Sizeof(v))
	       case  int64: return uint(unsafe.Sizeof(v))
	       case uint64: return uint(unsafe.Sizeof(v))
	       case string: return uint(len(v))
	       case []byte: return uint(len(v))
	       case Enum_SS_data_functions: 
			    return uint(unsafe.Sizeof(v))
	       default: fmt.Printf("Hitem: unexpected type %T\n", v)
        }
	return 0
}

func Hitem(skey uint64, sdata_type Enum_SS_datatype_t, s interface {}) Hskel {
        var data_ptr  unsafe.Pointer
	
        switch sdata_type {
		case TYPE_GOINTERFACET: data_ptr = unsafe.Pointer(ObjToPtr(s))
		case TYPE_RAWT:
			data_ptr  = hitem_getptr(s)
			data_len := hitem_getlen(s)

			d := Traw_t{ data_ptr, data_len }
			data_ptr = unsafe.Pointer( &d )
		default:
			data_ptr = hitem_getptr(s)
	}
	return Hskel{ skey, sdata_type, data_ptr }
}

func Hget(hash uintptr, skey uint64) interface {} {
	item := Hash_find(hash, skey)
	if item == 0 {
		return nil
	}
	data := Hash_item_data(item)
	switch t := Enum_SS_datatype_t(data.GetXtype()); t {
		case TYPE_GOINTERFACET: return ObjFromPtr( data.GetPtr() )
		case TYPE_INTT:         return  int(Go_data_to_uint(data))
		case TYPE_SIZET:        return uint(Go_data_to_uint(data))
		case TYPE_HASHKEYT:     return uint(Go_data_to_uint(data))
		case TYPE_UINTT:        return uint(Go_data_to_uint(data))
		case TYPE_RAWT:         s := []string{""}; Go_data_to_raw(data, s); return s[0]
		case TYPE_STRINGT:      s := []string{""}; Go_data_to_raw(data, s); return s[0]
		default: fmt.Printf("Hget: unexpected data type: %v\n", t)
	}
	return nil
}

func Hnext(hash uintptr) Hskel {
	return Hskel{ (^uint64(0))-1, 0, unsafe.Pointer(hash) }
}
func Hnull() Hskel {
        return Hskel{ (^uint64(0)), 0, unsafe.Pointer(nil) }
}
func Hend() Hskel {
        return Hskel{ (^uint64(0))-1, 0, unsafe.Pointer(nil) }
}
func Hash(hk []Hskel) uintptr {
        return uintptr(unsafe.Pointer(&hk[0]))
}

func init() {
	Frozen_init()
}

%}

#endif // }}}

/* vim: set filetype=c: */
