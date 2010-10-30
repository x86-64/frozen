#include <test.h>

static ssize_t test_rewrite(hash_t *rules, request_t *request, buffer_t *buffer){
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
				{ "rules",        DATA_PTR_HASHT(rules)                     },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	backend_t  *backend = backend_new(config);
	
	ret = backend_query(backend, request, buffer);
	backend_destroy(backend);
	return ret;
}

START_TEST (test_backend_rewrite){
	ssize_t   ret;
	buffer_t *buffer = buffer_alloc();
	
	hash_t  req_create[] = {
		{ "action", DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",   DATA_SIZET(10)                 },
		hash_end
	};
	
	// null request {{{
	hash_t  rules_none[] = { hash_end };
	ret = test_rewrite(rules_none, req_create, buffer);
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
	ret = test_rewrite(rules_unset_key, req_create, buffer);
		fail_unless(ret < 0, "backend rewrite rules unset_key failed\n");
	// }}}
	// unset buffer {{{
	hash_t  rules_unset_buf[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("unset") },
			{ "dst_buffer", DATA_INT32(1)        },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_unset_buf, req_create, buffer);
		fail_unless(ret == 0, "backend rewrite rules unset_buf failed\n");
	// }}}
	// test filter {{{
	hash_t  rules_unset_buf_filter[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("unset")           },
			{ "dst_buffer", DATA_INT32(1)                  },
			{ "on-request", DATA_INT32(ACTION_CRWD_CUSTOM) },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_unset_buf_filter, req_create, buffer);
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
	ret = test_rewrite(rules_set_key_from_key, req_create, buffer);
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
	ret = test_rewrite(rules_set_key_from_config, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules set_key_from_config 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_set_key_from_config, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules set_key_from_config 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key1)));
		fail_unless(key2 - key1 == 20, "backend rewrite rules set_key_from_config failed\n");
	// }}}
	// set key from buffer TODO {{{
	// }}}
	// set buffer from key {{{
	unsigned int bfk1;
	hash_t  rules_set_buffer_from_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",         DATA_STRING("set")             },
			{ "src_key",        DATA_STRING("size")            },
			{ "dst_buffer",     DATA_INT32(1)                  },
			{ "dst_buffer_off", DATA_OFFT(8)                   },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_set_buffer_from_key, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_set_buffer_from_key failed\n");
		buffer_read(buffer, 8, &bfk1, MIN(ret, sizeof(bfk1)));
		fail_unless(bfk1 == 10, "backend rewrite rules rules_set_buffer_from_key data failed\n");
	// }}}
	// set buffer from config key {{{
	unsigned int bfc1;
	hash_t  rules_set_buffer_from_config[] = {
		{ NULL, DATA_HASHT(
			{ "action",         DATA_STRING("set")             },
			{ "src_config",     DATA_STRING("default_size")    },
			{ "dst_buffer",     DATA_INT32(1)                  },
			{ "dst_buffer_off", DATA_OFFT(8)                   },
			
			{ "default_size",   DATA_SIZET(30)                 },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_set_buffer_from_config, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_set_buffer_from_config failed\n");
		buffer_read(buffer, 8, &bfc1, MIN(ret, sizeof(bfc1)));
		fail_unless(bfc1 == 30, "backend rewrite rules rules_set_buffer_from_config data failed\n");
	// }}}
	// set buffer from buffer {{{
	off_t bfb1, bfb2;
	hash_t  rules_set_buffer_from_buffer[] = {
		{ NULL, DATA_HASHT(
			{ "action",         DATA_STRING("set")             },
			{ "src_buffer",     DATA_INT32(1)                  },
			{ "dst_buffer",     DATA_INT32(1)                  },
			{ "dst_buffer_off", DATA_OFFT(8)                   },
			{ "after",          DATA_INT32(1)                  },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_set_buffer_from_buffer, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_set_buffer_from_buffer failed\n");
		buffer_read(buffer, 0,            &bfb1, MIN(ret, sizeof(bfb1)));
		buffer_read(buffer, 8,            &bfb2, MIN(ret, sizeof(bfb2)));
		fail_unless(bfb1 == bfb2, "backend rewrite rules rules_set_buffer_from_buffer data failed\n");
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
	ret = test_rewrite(rules_length_key, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_length_key 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_length_key, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_length_key 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 4, "backend rewrite rules rules_length_key failed\n");
	// }}}
	// calc length of buffer {{{
	hash_t  rules_length_buffer[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("length")          },
			{ "src_buffer", DATA_INT32(1)                  },
			{ "dst_key",    DATA_STRING("size")            },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_length_buffer, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_length_buffer 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_length_buffer, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_length_buffer 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 16, "backend rewrite rules rules_length_buffer failed\n");
	// }}}
	// calc data length of key {{{
	hash_t  rules_data_length_key[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("data_length")     },
			{ "type",       DATA_STRING("size_t")          },
			{ "src_config", DATA_STRING("default_size")    },
			{ "dst_key",    DATA_STRING("size")            },
			
			{ "default_size", DATA_SIZET(30)               },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_data_length_key, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_data_length_key 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_data_length_key, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_data_length_key 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 8, "backend rewrite rules rules_data_length_key failed\n");
	// }}}
	// calc data length of buffer {{{
	hash_t  rules_data_length_buffer[] = {
		{ NULL, DATA_HASHT(
			{ "action",     DATA_STRING("data_length")     },
			{ "type",       DATA_STRING("size_t")          },
			{ "src_buffer", DATA_INT32(1)                  },
			{ "dst_key",    DATA_STRING("size")            },
			hash_end
		)},
		hash_end
	};
	ret = test_rewrite(rules_data_length_buffer, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_data_length_buffer 1 failed\n");
		buffer_read(buffer, 0, &key1, MIN(ret, sizeof(key1)));
	ret = test_rewrite(rules_data_length_buffer, req_create, buffer);
		fail_unless(ret > 0, "backend rewrite rules rules_data_length_buffer 2 failed\n");
		buffer_read(buffer, 0, &key2, MIN(ret, sizeof(key2)));
		fail_unless(key2 - key1 == 8, "backend rewrite rules rules_data_length_buffer failed\n");
	// }}}
	

	buffer_free(buffer);
}
END_TEST
REGISTER_TEST(core, test_backend_rewrite)

