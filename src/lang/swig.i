%module Frozen

%include hashkeys.i
%include datatypes.i

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
//%typemap(gotype) data_t *,    const void * & "uintptr"
#endif

typedef enum data_functions {
	ACTION_CREATE,
	ACTION_DELETE,
	ACTION_MOVE,
	ACTION_COUNT,
	ACTION_CUSTOM,
	ACTION_REBUILD,

	ACTION_ALLOC,
	ACTION_FREE,
	ACTION_PHYSICALLEN,
	ACTION_LOGICALLEN,
	ACTION_CONVERTLEN,
	ACTION_COMPARE,
	ACTION_INCREMENT,
	ACTION_DECREMENT,
	ACTION_ADD,
	ACTION_SUB,
	ACTION_MULTIPLY,
	ACTION_DIVIDE,
	ACTION_READ,
	ACTION_WRITE,
	ACTION_CONVERT,
	ACTION_TRANSFER,
	ACTION_COPY,
	ACTION_IS_NULL,

	ACTION_GETDATAPTR,

	ACTION_INVALID
} data_functions;

typedef enum api_types {
	API_HASH = 1,
	API_CRWD = 2,
	API_FAST = 4
} api_types;

typedef hash_t                   request_t;
typedef signed int               ssize_t;
typedef unsigned int             size_t;
typedef signed long long int     intmax_t;
typedef unsigned long long int   uintmax_t;
typedef uintmax_t                hash_key_t;
typedef uintmax_t                data_type;

typedef int     (*f_init)      (backend_t *);
typedef int     (*f_fork)      (backend_t *, backend_t *, hash_t *);
typedef int     (*f_configure) (backend_t *, hash_t *);
typedef int     (*f_destroy)   (backend_t *);
typedef ssize_t (*f_crwd)      (backend_t *, request_t *);

struct backend_t {
	char                  *name;
	char                  *class;
	
	enum api_types         supported_api;
	f_init                 func_init;
	f_configure            func_configure;
	f_fork                 func_fork;
	f_destroy              func_destroy;
	
	struct {
		f_crwd  func_create;
		f_crwd  func_set;
		f_crwd  func_get;
		f_crwd  func_delete;
		f_crwd  func_move;
		f_crwd  func_count;
		f_crwd  func_custom;
	} backend_type_crwd;
	struct {
		f_crwd  func_handler;
	} backend_type_hash;

	void *                 userdata;
};
struct data_t {
	data_type       type;
	void           *ptr;
};

int                frozen_init(void);
int                frozen_destroy(void);

ssize_t            class_register               (backend_t *proto);
void               class_unregister             (backend_t *proto);

backend_t *        backend_new                  (hash_t *config);
backend_t *        backend_acquire              (char *name);
backend_t *        backend_find                 (char *name);
intmax_t           backend_query                (backend_t *backend, request_t *request);
void               backend_destroy              (backend_t *backend);

ssize_t            backend_pass                 (backend_t *backend, request_t *request);

hash_t *           configs_string_parse         (char *string);
hash_t *           configs_file_parse           (char *filename);

hash_t *           hash_new                     (size_t nelements);
hash_t *           hash_copy                    (hash_t *hash);
void               hash_free                    (hash_t *hash);

hash_t *           hash_find                    (hash_t *hash, hash_key_t key);
size_t             hash_nelements               (hash_t *hash);
#ifdef DEBUG
void               hash_dump                    (hash_t *hash);
#endif

hash_key_t         hash_item_key                (hash_t *hash);
data_t *           hash_item_data               (hash_t *hash);
data_t *           hash_data_find               (hash_t *hash, hash_key_t key);

data_type          data_type_from_string        (char *string);
char *             data_string_from_type        (data_type type);

const char *       describe_error               (intmax_t errnum);

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

//%cstring_output_allocate_size(char **string, size_t *len, );
//void               hash_get                     (hash_t *hash, char *key, char **string, size_t *len);
//ssize_t            hash_set                     (hash_t *hash, char *key, char *type, char *string);
//ssize_t            data_from_string             (data_t *data, char *type, char *string);

%inline %{
/*
ssize_t            data_from_string             (data_t *data, char *type, char *string){
        ssize_t   retval;
        data_type d_type = data_type_from_string(type);
        data_t    d_str  = DATA_PTR_STRING(string);
        
        data_convert_to_alloc(retval, d_type, data, &d_str, NULL);
        return retval;
}

ssize_t            hash_set                     (hash_t *hash, char *key, char *type, char *string){
        if( (hash = hash_find(hash, hash_ptr_null)) == NULL)
                return -EINVAL;
        
        hash->key = OLDOLDOLDhash_string_to_key(key);
        return data_from_string(hash_item_data(hash), type, string);
}

void               hash_get                     (hash_t *hash, char *key, char **string, size_t *len){
        data_t     *data;
        
        data = hash_data_find(hash, OLDOLDOLDhash_string_to_key(key));
        if(data == NULL)
                return;
        
        *string = data->ptr;
        *len    = strlen(data->ptr); // BAD BAD BAD WROOOONG
}
*/
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
void data_assign(data_t *data, data_type type, void *ptr){
	data->type = type;
	data->ptr  = ptr;
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
	Data_type     Enum_SS_data_type
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

func Hitem(skey uint64, sdata_type Enum_SS_data_type, s interface {}) Hskel {
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
	switch t := data.GetXtype(); t {
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
