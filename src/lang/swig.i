%module Frozen

%include hashkeys.i
%include datatypes.i

#ifndef SWIGGO
%include cstring.i
#else
#define DEBUG
#endif

#ifdef SWIGGO
%typemap(gotype) f_init,      const void * & "func(uintptr) int"
%typemap(gotype) f_fork,      const void * & "func(uintptr, uintptr, uintptr) int"
%typemap(gotype) f_configure, const void * & "func(uintptr, uintptr) int"
%typemap(gotype) f_destroy,   const void * & "func(uintptr) int"
%typemap(gotype) f_crwd,      const void * & "func(uintptr, uintptr) int"

%typemap(gotype) backend_t *, const void * & "uintptr"
%typemap(gotype) hash_t *,    const void * & "uintptr"
%typemap(gotype) request_t *, const void * & "uintptr"
%typemap(gotype) data_t *,    const void * & "uintptr"
#endif

%{
#include "libfrozen.h"
%}

enum request_actions {
	ACTION_CRWD_CREATE = 1,
	ACTION_CRWD_READ = 2,
	ACTION_CRWD_WRITE = 4,
	ACTION_CRWD_DELETE = 8,
	ACTION_CRWD_MOVE = 16,
	ACTION_CRWD_COUNT = 32,
	ACTION_CRWD_CUSTOM = 64,
	
	REQUEST_INVALID = 0
};

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
	
	uintmax_t              supported_api;
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
};

int                frozen_init(void);
int                frozen_destroy(void);

void            backend_test(backend_t *func);
int             backend_test_pass(backend_t *backend, hash_t *request);

backend_t *        backend_new                  (hash_t *config);
backend_t *        backend_acquire              (char *name);
backend_t *        backend_find                 (char *name);
intmax_t           backend_query                (backend_t *backend, request_t *request);
void               backend_destroy              (backend_t *backend);

hash_t *           configs_string_parse         (char *string);
hash_t *           configs_file_parse           (char *filename);

hash_t *           hash_new                     (size_t nelements);
hash_t *           hash_copy                    (hash_t *hash);
void               hash_free                    (hash_t *hash);

hash_t *           hash_find                    (hash_t *hash, hash_key_t key);
void               hash_chain                   (hash_t *hash, hash_t *hash_next);
size_t             hash_nelements               (hash_t *hash);
#ifdef DEBUG
void               hash_dump                    (hash_t *hash);
#endif

hash_key_t         hash_string_to_key           (char *string);
char *             hash_key_to_string           (hash_key_t key);
hash_key_t         hash_key_to_ctx_key          (hash_key_t key);

hash_key_t         hash_item_key                (hash_t *hash);
data_t *           hash_item_data               (hash_t *hash);
hash_t *           hash_item_next               (hash_t *hash);
void               hash_data_find               (hash_t *hash, hash_key_t key, data_t **data, data_ctx_t **data_ctx);

data_type          data_type_from_string        (char *string);
char *             data_string_from_type        (data_type type);
void               data_free                    (data_t *data);

#ifndef SWIGGO
%cstring_output_allocate_size(char **string, size_t *len, );
#endif
void               hash_get                     (hash_t *hash, char *key, char **string, size_t *len);
ssize_t            hash_set                     (hash_t *hash, char *key, char *type, char *string);
ssize_t            data_from_string             (data_t *data, char *type, char *string);

const char *       describe_error               (intmax_t errnum);

%inline %{
ssize_t            data_from_string             (data_t *data, char *type, char *string){
        ssize_t   retval;
        data_type d_type = data_type_from_string(type);
        data_t    d_str  = DATA_PTR_STRING(string, strlen(string)+1);
        
        data_convert_to_alloc(retval, d_type, data, &d_str, NULL);
        return retval;
}

ssize_t            hash_set                     (hash_t *hash, char *key, char *type, char *string){
        if( (hash = hash_find(hash, hash_ptr_null)) == NULL)
                return -EINVAL;
        
        hash->key = hash_string_to_key(key);
        return data_from_string(hash_item_data(hash), type, string);
}

void               hash_get                     (hash_t *hash, char *key, char **string, size_t *len){
        data_t     *data;
        
        hash_data_find(hash, hash_string_to_key(key), &data, NULL);
        if(data == NULL)
                return;
        
        *string = data_value_ptr(data);
        *len    = data_value_len(data);
}

%}

// temprorary solution, need remove data_convert

#ifdef SWIGPERL

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

#endif

#ifdef SWIGGO


%insert("go_begin") %{

import "unsafe"

%}

%insert("go_wrapper") %{

type Hskel struct {
	Key           uint64
	Data_type     Enum_SS_data_type
	Data_ptr      unsafe.Pointer
	Data_length   uint64
}; 

func Hitem(skey uint64, sdata_type Enum_SS_data_type, s interface {}) Hskel {
        var data_ptr  unsafe.Pointer
        var data_len  uint64

        switch v := s.(type) {
                case  int:   data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case uint:   data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case  int16: data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case uint16: data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case  int32: data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case uint32: data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case  int64: data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case uint64: data_ptr, data_len = unsafe.Pointer(&v), uint64(unsafe.Sizeof(v))
                case string: data_ptr, data_len = unsafe.Pointer(&([]byte( v )[0])), uint64(len(v))
		case []byte: data_ptr, data_len = unsafe.Pointer(&v[0]), uint64(len(v))
        }
        return Hskel{ skey, sdata_type, data_ptr, data_len }
}
func Hnext(hash uintptr) Hskel {
	return Hskel{ (^uint64(0))-1, 0, unsafe.Pointer(hash), 0 }
}
func Hnull() Hskel {
        return Hskel{ (^uint64(0)), 0, unsafe.Pointer(nil), 0 }
}
func Hend() Hskel {
        return Hskel{ (^uint64(0))-1, 0, unsafe.Pointer(nil), 0 }
}
func Hash(hk []Hskel) uintptr {
        return uintptr(unsafe.Pointer(&hk[0]))
}

func init() {
	Frozen_init()
}

%}

#endif

/* vim: set filetype=c: */
