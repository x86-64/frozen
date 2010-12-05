#include <test.h>

START_TEST (test_real_store_nums){
	hash_t config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                        },
				{ "filename",  DATA_STRING("data_real_store_nums.dat")    },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *backend = backend_new(config);
	
	off_t data_ptrs[6];
	int   data_array[] = { 100, 200, 500, 800, 888, 900 };
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)  },
			{ "key_out", DATA_PTR_OFFT  (&data_ptrs[i])  },
			{ "buffer",  DATA_PTR_INT32 (&data_array[i]) },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret == 4, "chain 'real_store_nums': write array failed");
	}
	
	// check
	int data_read;
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)     },
			{ "key",    DATA_OFFT(data_ptrs[i])          },
			{ "buffer", DATA_PTR_INT32(&data_read)       },
			hash_end
		};
		ret = backend_query(backend, r_read);
			fail_unless(ret == 4,                   "chain 'real_store_nums': read array failed");
			fail_unless(data_read == data_array[i], "chain 'real_store_nums': read array data failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_real_store_nums)

START_TEST (test_real_store_strings){
	hash_t config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                        },
				{ "filename",  DATA_STRING("data_real_store_strings.dat") },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *backend = backend_new(config);
	
	off_t  data_ptrs[6];
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ "key_out", DATA_PTR_OFFT   (&data_ptrs[i])                          },
			{ "buffer",  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret > 0, "chain 'real_store_strings': write array failed");
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)      },
			{ "key",    DATA_OFFT(data_ptrs[i])           },
			{ "buffer", DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(backend, r_read);
			fail_unless(ret > 0,                               "chain 'real_store_strings': read array failed");
			fail_unless(strcmp(data_read, data_array[i]) == 0, "chain 'real_store_strings': read array data failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_real_store_strings)

START_TEST (test_real_store_idx_strings){
	hash_t c_data[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                           },
				{ "filename",  DATA_STRING("data_real_store_dat_string.dat") },
				hash_end
			)},
			hash_end
		)},
		{ "name", DATA_STRING("b_data") },
		hash_end
	};
	backend_t  *b_data = backend_new(c_data);
	
	hash_t c_idx[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("file")                               },
				{ "filename",     DATA_STRING("data_real_store_idx_string.dat")     },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("rewrite")                            },
				{ "rules",        DATA_HASHT(
					{ NULL, DATA_HASHT( // 1
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("*")                    },
						{ "dst_key",    DATA_STRING("key")                  },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						{ "copy",       DATA_INT32(1)                       },
						{ "on-request", DATA_INT32(
							ACTION_CRWD_READ |
							ACTION_CRWD_WRITE |
							ACTION_CRWD_DELETE
						)},
						hash_end
					)},
					// move {{{
					{ NULL, DATA_HASHT( // 2
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("*")                    },
						{ "dst_key",    DATA_STRING("key_from")             },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						//{ "copy",       DATA_INT32(1)                       },
						{ "on-request", DATA_INT32(ACTION_CRWD_MOVE)        },
						hash_end
					)},
					{ NULL, DATA_HASHT( // 3
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("*")                    },
						{ "dst_key",    DATA_STRING("key_to")               },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						//{ "copy",       DATA_INT32(1)                       },
						{ "on-request", DATA_INT32(ACTION_CRWD_MOVE)        },
						hash_end
					)},
					// }}}
					// write {{{
					{ NULL, DATA_HASHT( // 4
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("buffer")            },
						{ "dst_backend",   DATA_STRING("buffer")            },
						{ "dst_rule",      DATA_INT32(6)                    },
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)},
					{ NULL, DATA_HASHT( // 5
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key_out")           },
						{ "dst_backend",   DATA_STRING("key_out")           },
						{ "dst_rule",      DATA_INT32(6)                    },
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)},
					{ NULL, DATA_HASHT( // 6
						{ "action",          DATA_STRING("backend")         },
						{ "backend",         DATA_STRING("b_data")          },
						{ "request_proto",   DATA_HASHT(
							{ "action",  DATA_INT32(ACTION_CRWD_WRITE)  },
							hash_end
						)},
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)},
					{ NULL, DATA_HASHT( // 7
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key_out")           },
						{ "dst_key",       DATA_STRING("buffer")            },
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)},
					{ NULL, DATA_HASHT( // 8
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("/")                    },
						{ "dst_key",    DATA_STRING("key_out")              },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						{ "after",      DATA_INT32(1)                       },
						{ "on-request", DATA_INT32(
							ACTION_CRWD_CREATE | ACTION_CRWD_WRITE)     },
						hash_end
					)},
					// }}}
					// read {{{
					{ NULL, DATA_HASHT( // 9
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("buffer")            },
						{ "src_info",      DATA_INT32(1)                    },
						{ "dst_key",       DATA_STRING("key")               },
						{ "dst_info",      DATA_INT32(1)                    },
						{ "on-request",    DATA_INT32(ACTION_CRWD_READ)     },
						{ "after",         DATA_INT32(1)                    },
						hash_end
					)},
					{ NULL, DATA_HASHT( // 10
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key")               },
						{ "dst_backend",   DATA_STRING("key")               },
						{ "dst_rule",      DATA_INT32(12)                   },
						{ "on-request",    DATA_INT32(ACTION_CRWD_READ)     },
						{ "after",         DATA_INT32(1)                    },
						hash_end
					)},
					{ NULL, DATA_HASHT( // 11
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("buffer")            },
						{ "dst_backend",   DATA_STRING("buffer")            },
						{ "dst_rule",      DATA_INT32(12)                   },
						{ "on-request",    DATA_INT32(ACTION_CRWD_READ)     },
						{ "after",         DATA_INT32(1)                    },
						{ "fatal",         DATA_INT32(1)                    },
						hash_end
					)},
					{ NULL, DATA_HASHT( // 12
						{ "action",          DATA_STRING("backend")         },
						{ "backend",         DATA_STRING("b_data")          },
						{ "request_proto",   DATA_HASHT(
							{ "action",  DATA_INT32(ACTION_CRWD_READ)   },
							hash_end
						)},
						{ "on-request",      DATA_INT32(ACTION_CRWD_READ)   },
						{ "after",           DATA_INT32(1)                  },
						hash_end
					)},
					// }}}
					// count {{{
					{ NULL, DATA_HASHT( // 13
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("/")                    },
						{ "dst_key",    DATA_STRING("buffer")               },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						{ "after",      DATA_INT32(1)                       },
						{ "on-request", DATA_INT32(ACTION_CRWD_COUNT)       },
						hash_end
					)},
					// }}}
					hash_end
				)},
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("list")                               },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("insert-sort")                        },
				{ "engine",       DATA_STRING("binsearch")                          },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *b_idx = backend_new(c_idx);
	
	off_t  data_ptrs[6];
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ "key_out", DATA_PTR_OFFT   (&data_ptrs[i])                          },
			{ "buffer",  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(b_idx, r_write);
			fail_unless(ret > 0,           "chain 'real_store_idx_str': write array failed");
			//fail_unless(data_ptrs[i] == i, "chain 'real_store_idx_str': write index not linear");
		
		printf("ret: %x, ptr: %d\n", ret, (unsigned int)data_ptrs[i]);
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(data_read, 0, 1024);

		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)      },
			{ "key",    DATA_OFFT(i)                      },
			{ "buffer", DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(b_idx, r_read);
			fail_unless(ret > 0,                               "chain 'real_store_idx_str': read array failed");
			//fail_unless(strcmp(data_read, data_array[i]) == 0, "chain 'real_store_idx_str': read array data failed");
		
		printf("ret: %x, str: %s\n", ret, data_read);
	}
	
	backend_destroy(b_idx);
	backend_destroy(b_data);
}
END_TEST
REGISTER_TEST(core, test_real_store_idx_strings)

/*
START_TEST (test_real_store_idx_strings){
	hash_t c_idx[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("file")                               },
				{ "filename",     DATA_STRING("data_real_store_idx_string.dat")     },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("rewrite")                            },
				{ "rules",        DATA_HASHT(
					{ NULL, DATA_HASHT(
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("/")                    },
						{ "dst_key",    DATA_STRING("key_out")              },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						{ "after",      DATA_INT32(1)                       },
						{ "on-request", DATA_INT32(
							ACTION_CRWD_CREATE | ACTION_CRWD_WRITE)     },
						hash_end
					)},
					{ NULL, DATA_HASHT(
						{ "action",     DATA_STRING("data_arith")           },
						{ "operator",   DATA_STRING("*")                    },
						{ "dst_key",    DATA_STRING("key")                  },
						{ "src_config", DATA_STRING("operand")              },
						{ "operand",    DATA_OFFT(8)                        },
						{ "on-request", DATA_INT32(ACTION_CRWD_READ)        },
						hash_end
					)},
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		{ "name", DATA_STRING("str_idx") },
		hash_end
	};
	backend_t  *b_idx = backend_new(c_idx);
	
	hash_t c_data[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                           },
				{ "filename",  DATA_STRING("data_real_store_dat_string.dat") },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("rewrite")                     },
				{ "rules",     DATA_HASHT(
					// create and write {{{
					{ NULL, DATA_HASHT(
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key_out")           },
						{ "dst_backend",   DATA_STRING("buffer")            },
						{ "dst_rule",      DATA_INT32(3)                    },
						{ "after",         DATA_INT32(1)                    },
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)},
					{ NULL, DATA_HASHT(
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key_out")           },
						{ "dst_backend",   DATA_STRING("key_out")           },
						{ "dst_rule",      DATA_INT32(3)                    },
						{ "after",         DATA_INT32(1)                    },
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)},
					{ NULL, DATA_HASHT( // call backend
						{ "action",        DATA_STRING("backend")           },
						{ "backend",       DATA_STRING("str_idx")           },
						{ "request_proto", DATA_HASHT(
							{ "action",  DATA_INT32(ACTION_CRWD_WRITE)  },
							hash_end
						)},
						{ "after",         DATA_INT32(1)                    },
						{ "on-request",
							DATA_INT32(
								ACTION_CRWD_CREATE | ACTION_CRWD_WRITE
							)},
						hash_end
					)}, // }}}
					// read {{{
					{ NULL, DATA_HASHT(
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key")               },
						{ "dst_backend",   DATA_STRING("key")               },
						{ "dst_rule",      DATA_INT32(6)                    },
						{ "on-request",    DATA_INT32(ACTION_CRWD_READ)     },
						hash_end
					)},
					{ NULL, DATA_HASHT(
						{ "action",        DATA_STRING("set")               },
						{ "src_key",       DATA_STRING("key")               },
						{ "dst_backend",   DATA_STRING("buffer")            },
						{ "dst_rule",      DATA_INT32(6)                    },
						{ "on-request",    DATA_INT32(ACTION_CRWD_READ)     },
						hash_end
					)},
					{ NULL, DATA_HASHT( // call backend
						{ "action",        DATA_STRING("backend")           },
						{ "backend",       DATA_STRING("str_idx")           },
						{ "request_proto", DATA_HASHT(
							{ "action",  DATA_INT32(ACTION_CRWD_READ)   },
							hash_end
						)},
						{ "on-request",    DATA_INT32(ACTION_CRWD_READ)     },
						hash_end
					)},
					// }}}
					hash_end
				)},
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *b_data = backend_new(c_data);
	
	off_t  data_ptrs[6];
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ "key_out", DATA_PTR_OFFT   (&data_ptrs[i])                          },
			{ "buffer",  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(b_data, r_write);
			fail_unless(ret > 0,           "chain 'real_store_idx_str': write array failed");
			fail_unless(data_ptrs[i] == i, "chain 'real_store_idx_str': write index not linear");
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)      },
			{ "key",    DATA_OFFT(data_ptrs[i])           },
			{ "buffer", DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(b_data, r_read);
			fail_unless(ret > 0,                               "chain 'real_store_idx_str': read array failed");
			fail_unless(strcmp(data_read, data_array[i]) == 0, "chain 'real_store_idx_str': read array data failed");
		
		//printf("ret: %x, str: %s\n", ret, data_read);
	}
	
	backend_destroy(b_data);
	backend_destroy(b_idx);
}
END_TEST
RRRRRRRREGISTER_TEST(core, test_real_store_idx_strings)
*/
