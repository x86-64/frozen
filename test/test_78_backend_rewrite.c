
off_t     buffer;

static ssize_t test_rewrite(char *rules, request_t *request){
	ssize_t ret;
	hash_t  config[] = {
		{ 0, DATA_HASHT(
			{ HK(class),        DATA_STRING("file")                       },
			{ HK(filename),     DATA_STRING("data_backend_rewrite.dat")   },
			hash_end
		)},
		{ 0, DATA_HASHT(
			{ HK(class),        DATA_STRING("rewrite")                    },
			{ HK(script),       DATA_PTR_STRING(rules)                    },
			hash_end
		)},
		hash_end
	};
	backend_t  *backend = backend_new(config);
	
	request_t new_request[] = {
                { HK(ret), DATA_PTR_SIZET(&ret) },
                hash_next(request)
        };

        backend_query(backend, new_request);
	backend_destroy(backend);
	return ret;
}

static size_t test_diff_rewrite(char *rules, request_t *request){
	off_t   key1, key2;
	
	test_rewrite(rules, request);
		key1 = buffer;
	test_rewrite(rules, request);
		key2 = buffer;
	
	return (key2-key1);
}

START_TEST (test_backend_rewrite){
	ssize_t   ret;
	
	hash_t  req_create[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CREATE) },
		{ HK(size),       DATA_SIZET(10)                   },
		{ HK(offset_out), DATA_PTR_OFFT(&buffer)           },
		hash_end
	};
	
	// null request {{{
	ret = test_rewrite("", req_create);
		fail_unless(ret >= 0, "backend rewrite rules none failed\n");
	// }}}
	// unset key {{{
	//char rules_unset_key[] =
	//	"request['size'] = (void_t)''; "
	//	"ret = pass(request);";
	//
        //ret = test_rewrite(rules_unset_key, req_create);
	//	fail_unless(ret < 0, "backend rewrite rules unset_key failed\n");
	// }}}
	// set key from key {{{
	char rules_set_key_from_key[] =
		"request['size'] = request['size'];"
		"ret = pass(request);";
	ret = test_rewrite(rules_set_key_from_key, req_create);
		fail_unless(ret >= 0, "backend rewrite rules set_key_from_key failed\n");
	// }}}
	// set key from config key {{{
	char rules_set_key_from_config[] =
		"request['size'] = (size_t)'20';"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_set_key_from_config, req_create);
		fail_unless(ret == 20, "backend rewrite rules set_key_from_config failed\n");
	// }}}
	// do backend call {{{
	hash_t  rewrite_be_test_conf[] = {
		{ 0, DATA_HASHT(
			{ HK(name),       DATA_STRING("rewrite_be_test")                    },
			{ HK(class),      DATA_STRING("file")                               },
		        { HK(filename),   DATA_STRING("data_backend_rewrite_backend.dat")   },
			hash_end
		)},
		hash_end
	};
	backend_t *rewrite_be_test = backend_new(rewrite_be_test_conf);
	
	char rules_call_backend[] =
		"request_t subr; "
		
		"subr['action']  = create; "
		"subr['size']    = (size_t)'35'; "
		
		"subr['offset_out'] = (size_t)'0'; "
		
		"query((backend_t)'rewrite_be_test', subr); " // returns 0
		"query((backend_t)'rewrite_be_test', subr); " // returns 35
		
		"request['size'] = subr['offset_out']; "
		
		"ret = pass(request); "
		;
		
	ret = test_diff_rewrite(rules_call_backend, req_create);
		fail_unless(ret == 35, "backend rewrite rules rules_call_backend failed\n");
	
	backend_destroy(rewrite_be_test);
	// }}}
	// if's {{{
	char rules_if_1[] =
		"ret = (size_t)'0'; if( (size_t)'0' ){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_1, req_create);
		fail_unless(ret == 0, "backend rewrite rules if_1 failed\n");
	
	char rules_if_2[] =
		"if( (size_t)'20' ){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_2, req_create);
		fail_unless(ret == 10, "backend rewrite rules if_2 failed\n");
	// real
	char rules_if_real_1[] =
		"ifnot( data_query( request['action'], compare, create ) ){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_real_1, req_create);
		fail_unless(ret == 10, "backend rewrite rules if_real_1 failed\n");
	
	char rules_if_real_2[] =
		"if( data_query( request['action'], compare, delete )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_real_2, req_create);
		fail_unless(ret == 10, "backend rewrite rules if_real_2 failed\n");
	// }}}
}
END_TEST
REGISTER_TEST(core, test_backend_rewrite)

