#include <test.h>

static ssize_t test_rewrite(char *rules, request_t *request){
	ssize_t ret;
	hash_t  config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("file")                       },
				{ "filename",     DATA_STRING("data_backend_rewrite.dat")   },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("rewrite")                    },
				{ "script",       DATA_STRING(rules)                        },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	backend_t  *backend = backend_new(config);
	
	ret = backend_query(backend, request);
	backend_destroy(backend);
	return ret;
}

static size_t test_diff_rewrite(char *rules, request_t *request, buffer_t *buffer){
	off_t   key1, key2;
	ssize_t ret;
	
	ret = test_rewrite(rules, request);
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules, request);
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
	
	return (key2-key1);
}

START_TEST (test_backend_rewrite){
	ssize_t   ret;
	buffer_t *buffer = buffer_alloc();
	
	hash_t  req_create[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",    DATA_SIZET(10)                 },
		{ "key_out", DATA_BUFFERT(buffer)           },
		hash_end
	};
	
	// null request {{{
	ret = test_rewrite("", req_create);
		fail_unless(ret > 0, "backend rewrite rules none failed\n");
	// }}}
	// unset key {{{
	char rules_unset_key[] =
		"request['size'] = (void)''; "
		"ret = pass(request);";
	
	ret = test_rewrite(rules_unset_key, req_create);
		fail_unless(ret < 0, "backend rewrite rules unset_key failed\n");
	// }}}
	// set key from key {{{
	char rules_set_key_from_key[] =
		"request['size'] = request['size'];"
		"ret = pass(request);";
	ret = test_rewrite(rules_set_key_from_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules set_key_from_key failed\n");
	// }}}
	// set key from config key {{{
	char rules_set_key_from_config[] =
		"request['size'] = (size_t)'20';"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_set_key_from_config, req_create, buffer);
		fail_unless(ret == 20, "backend rewrite rules set_key_from_config failed\n");
	// }}}
	// calc length of key {{{
	char rules_length_key[] =
		"request['size'] = length((size_t)'30');"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_length_key, req_create, buffer);
		fail_unless(ret == 4, "backend rewrite rules rules_length_key failed\n");
	// }}}
	// calc data length of key {{{
	char rules_data_length_key[] =
		"request['size'] = data_length((size_t)'30');"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_data_length_key, req_create, buffer);
		fail_unless(ret == 4, "backend rewrite rules rules_data_length_key failed\n");
	// }}}
	// do arith of key {{{
	char rules_arith_key[] =
		"data_arith((string)'+', request['size'], (size_t)'30');"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_arith_key, req_create, buffer);
		fail_unless(ret == 40, "backend rewrite rules rules_arith_key failed\n");
	// }}}
	/*
	// do backend call {{{
	off_t  key_out = 150;
	hash_t  rules_call_backend[] = {
		{ NULL, DATA_HASHT( // set rule #2 request's "size"=30
			{ "action",        DATA_STRING("set")             },
			{ "src_config",    DATA_STRING("default_size")    },
			{ "dst_backend",   DATA_STRING("size")            },
			{ "dst_rule",      DATA_INT32(2)                  },
			
			{ "default_size",  DATA_SIZET(30)                 },
			hash_end
		)},
		{ NULL, DATA_HASHT( // call backend
			{ "action",        DATA_STRING("backend")         },
			{ "backend",       DATA_HASHT(
				{ NULL, DATA_HASHT(
					{ "name",         DATA_STRING("file")                               },
					{ "filename",     DATA_STRING("data_backend_rewrite_backend.dat")   },
					hash_end
				)},
				hash_end
			)},
			{ "request_proto", DATA_HASHT(
				{ "action",  DATA_INT32(ACTION_CRWD_CREATE) },
				{ "key_out", DATA_PTR_SIZET(&key_out)        },
				hash_end
			)},
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "action",        DATA_STRING("set")             },
			{ "src_backend",   DATA_STRING("key_out")         },
			{ "src_rule",      DATA_INT32(2)                  },
			{ "dst_key",       DATA_STRING("size")            },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_call_backend, req_create); // first call will be with "size"=0
		fail_unless(ret > 0, "backend rewrite rules rules_call_backend 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_call_backend, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_call_backend 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 0, "backend rewrite rules rules_call_backend failed\n");
	// }}}
	*/

	buffer_free(buffer);
}
END_TEST
REGISTER_TEST(core, test_backend_rewrite)

/*
START_TEST (test_backend_rewrite){
	ssize_t   ret;
	buffer_t *buffer = buffer_alloc();
	
	hash_t  req_create[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",    DATA_SIZET(10)                 },
		{ "key_out", DATA_BUFFERT(buffer)           },
		hash_end
	};
	
	// null request {{{
	hash_t  rules_none[] = { hash_end };
	ret = test_rewrite(rules_none, req_create);
		fail_unless(ret > 0, "backend rewrite rules none failed\n");
	// }}}
	// unset key {{{
	hash_t  rules_unset_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",  DATA_STRING("unset") },
			{ "dst_key", DATA_STRING("size")  },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_unset_key, req_create);
		fail_unless(ret < 0, "backend rewrite rules unset_key failed\n");
	// }}}
	// test filter {{{
	hash_t  rules_unset_buf_filter[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("unset")           },
			{ "dst_key",    DATA_STRING("size")            },
			{ "on-request", DATA_INT32(ACTION_CRWD_CUSTOM) },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_unset_buf_filter, req_create);
		fail_unless(ret > 0, "backend rewrite rules unset_buf_filter failed\n");
	// }}}
	// set key from key {{{
	hash_t  rules_set_key_from_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("set")             },
			{ "src_key",    DATA_STRING("size")            },
			{ "dst_key",    DATA_STRING("size")            },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_set_key_from_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules set_key_from_key failed\n");
	// }}}
	// set key from config key {{{
	off_t   key1, key2;
	hash_t  rules_set_key_from_config[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("set")             },
			{ "src_config", DATA_STRING("default_size")    },
			{ "dst_key",    DATA_STRING("size")            },
			
			{ "default_size", DATA_SIZET(20)               },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_set_key_from_config, req_create);
		fail_unless(ret > 0, "backend rewrite rules set_key_from_config 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_set_key_from_config, req_create);
		fail_unless(ret > 0, "backend rewrite rules set_key_from_config 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key1)));
		fail_unless(key2 - key1 == 20, "backend rewrite rules set_key_from_config failed\n");
	// }}}
	// calc length of key {{{
	hash_t  rules_length_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("length")          },
			{ "src_config", DATA_STRING("default_size")    },
			{ "dst_key",    DATA_STRING("size")            },
			
			{ "default_size", DATA_SIZET(30)               },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_length_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_length_key 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_length_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_length_key 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 4, "backend rewrite rules rules_length_key failed\n");
	// }}}
	// calc data length of key {{{
	hash_t  rules_data_length_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("data_length")     },
			{ "src_config", DATA_STRING("default_size")    },
			{ "dst_key",    DATA_STRING("size")            },
			
			{ "default_size", DATA_SIZET(30)               },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_data_length_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_data_length_key 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_data_length_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_data_length_key 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 4, "backend rewrite rules rules_data_length_key failed\n");
	// }}}
	// do arith of key {{{
	hash_t  rules_arith_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("data_arith")      },
			{ "dst_key",    DATA_STRING("size")            },
			{ "src_config", DATA_STRING("default_size")    },
			{ "operator",   DATA_STRING("+")               },
			{ "copy",       DATA_INT32(1)                  },
			
			{ "default_size", DATA_SIZET(30)               },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_arith_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_arith_key 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_arith_key, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_arith_key 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 40, "backend rewrite rules rules_arith_key failed\n");
	// }}}
	// do backend call {{{
	off_t  key_out = 150;
	hash_t  rules_call_backend[] = {
		{ NULL, DATA_HASHT( // set rule #2 request's "size"=30
			{ "action",        DATA_STRING("set")             },
			{ "src_config",    DATA_STRING("default_size")    },
			{ "dst_backend",   DATA_STRING("size")            },
			{ "dst_rule",      DATA_INT32(2)                  },
			
			{ "default_size",  DATA_SIZET(30)                 },
			hash_end
		)},
		{ NULL, DATA_HASHT( // call backend
			{ "action",        DATA_STRING("backend")         },
			{ "backend",       DATA_HASHT(
				{ NULL, DATA_HASHT(
					{ "name",         DATA_STRING("file")                               },
					{ "filename",     DATA_STRING("data_backend_rewrite_backend.dat")   },
					hash_end
				)},
				hash_end
			)},
			{ "request_proto", DATA_HASHT(
				{ "action",  DATA_INT32(ACTION_CRWD_CREATE) },
				{ "key_out", DATA_PTR_SIZET(&key_out)        },
				hash_end
			)},
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "action",        DATA_STRING("set")             },
			{ "src_backend",   DATA_STRING("key_out")         },
			{ "src_rule",      DATA_INT32(2)                  },
			{ "dst_key",       DATA_STRING("size")            },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_call_backend, req_create); // first call will be with "size"=0
		fail_unless(ret > 0, "backend rewrite rules rules_call_backend 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_call_backend, req_create);
		fail_unless(ret > 0, "backend rewrite rules rules_call_backend 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 0, "backend rewrite rules rules_call_backend failed\n");
	// }}}
	
	buffer_free(buffer);
}
END_TEST
RRRRRRRRRREGISTER_TEST(core, test_backend_rewrite)
*/
