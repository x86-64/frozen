
off_t     buffer;

static ssize_t test_rewrite(char *rules, request_t *request){
	ssize_t ret;
	hash_t  config[] = {
		{ HK(chains), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),         DATA_STRING("file")                       },
				{ HK(filename),     DATA_STRING("data_backend_rewrite.dat")   },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),         DATA_STRING("rewrite")                    },
				{ HK(script),       DATA_STRING(rules)                        },
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

static size_t test_diff_rewrite(char *rules, request_t *request){
	off_t   key1, key2;
	ssize_t ret;
	
	ret = test_rewrite(rules, request);
		key1 = buffer;
	ret = test_rewrite(rules, request);
		key2 = buffer;
	
	return (key2-key1);
}

START_TEST (test_backend_rewrite){
	ssize_t   ret;
	
	hash_t  req_create[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_CREATE) },
		{ HK(size),       DATA_SIZET(10)                 },
		{ HK(offset_out), DATA_PTR_OFFT(&buffer)         },
		hash_end
	};
	
	// null request {{{
	ret = test_rewrite("", req_create);
		fail_unless(ret >= 0, "backend rewrite rules none failed\n");
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
		fail_unless(ret >= 0, "backend rewrite rules set_key_from_key failed\n");
	// }}}
	// set key from config key {{{
	char rules_set_key_from_config[] =
		"request['size'] = (size_t)'20';"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_set_key_from_config, req_create);
		fail_unless(ret == 20, "backend rewrite rules set_key_from_config failed\n");
	// }}}
	// calc length of key {{{
	char rules_length_key[] =
		"request['size'] = length((size_t)'30');"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_length_key, req_create);
		fail_unless(ret == 4 || ret == 8, "backend rewrite rules rules_length_key failed\n");
	// }}}
	// calc data length of key {{{
	char rules_data_length_key[] =
		"request['size'] = data_length((size_t)'30');"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_data_length_key, req_create);
		fail_unless(ret == 4 || ret == 8, "backend rewrite rules rules_data_length_key failed\n");
	// }}}
	// do arith of key {{{
	char rules_arith_key[] =
		"data_arith((string)'+', request['size'], (size_t)'30');"
		"ret = pass(request);";
	ret = test_diff_rewrite(rules_arith_key, req_create);
		fail_unless(ret == 40, "backend rewrite rules rules_arith_key failed\n");
	// }}}
	// do backend call {{{
	hash_t  rewrite_be_test_conf[] = {
		{ HK(name),   DATA_STRING("rewrite_be_test") },
		{ HK(chains), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),         DATA_STRING("file")                               },
				{ HK(filename),     DATA_STRING("data_backend_rewrite_backend.dat")   },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	backend_t *rewrite_be_test = backend_new(rewrite_be_test_conf);
	
	char rules_call_backend[] =
		"request_t subr; "
		
		"subr['action']  = create; "
		"subr['size']    = (size_t)'35'; "
		
		"subr['offset_out'] = data_alloca((string)'size_t', (size_t)'8'); "
		
		"backend((string)'rewrite_be_test', subr); " // returns 0
		"backend((string)'rewrite_be_test', subr); " // returns 35
		
		"request['size'] = subr['offset_out']; "
		
		"ret = pass(request); "
		;
		
	ret = test_diff_rewrite(rules_call_backend, req_create);
		fail_unless(ret == 35, "backend rewrite rules rules_call_backend failed\n");
	
	backend_destroy(rewrite_be_test);
	// }}}
	// if's {{{
	// not obvious compares, but data_cmp return 0 on equal data
	char rules_if_1[] =
		"if(data_cmp( (size_t)'10', (size_t)'10' )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_1, req_create);
		fail_unless(ret == 0, "backend rewrite rules if_1 failed\n");
	
	char rules_if_2[] =
		"if(data_cmp( (size_t)'10', (size_t)'20' )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_2, req_create);
		fail_unless(ret == 10, "backend rewrite rules if_2 failed\n");
	// negs
	char rules_if_neg_1[] =
		"if(!data_cmp( (size_t)'10', (size_t)'10' )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_neg_1, req_create);
		fail_unless(ret == 10, "backend rewrite rules if_neg_1 failed\n");
	
	char rules_if_neg_2[] =
		"if(!data_cmp( (size_t)'10', (size_t)'20' )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_neg_2, req_create);
		fail_unless(ret == 0, "backend rewrite rules if_neg_2 failed\n");
	
	// real
	char rules_if_real_1[] =
		"if(!data_cmp( request['action'], create )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_real_1, req_create);
		fail_unless(ret == 10, "backend rewrite rules if_real_1 failed\n");
	
	char rules_if_real_2[] =
		"if(!data_cmp( request['action'], delete )){ ret = (size_t)'10'; };";
	
	ret = test_rewrite(rules_if_real_2, req_create);
		fail_unless(ret == 0, "backend rewrite rules if_real_2 failed\n");
	// }}}
	
}
END_TEST
REGISTER_TEST(core, test_backend_rewrite)

