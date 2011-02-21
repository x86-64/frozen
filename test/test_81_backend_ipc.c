
START_TEST(test_backend_ipc){
	backend_t  *backend;
	
	hash_t  settings[] = {
		{ HK(chains), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("file")                        },
				{ HK(filename),    DATA_STRING("data_backend_ipc.dat")        },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("ipc")                         },
				{ HK(type),        DATA_STRING("shmem")                       },
				{ HK(role),        DATA_STRING("server")                      },
				{ HK(key),         DATA_INT32(1000)                           },
				{ HK(size),        DATA_SIZET(10)                             },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("ipc")                         },
				{ HK(type),        DATA_STRING("shmem")                       },
				{ HK(role),        DATA_STRING("client")                      },
				{ HK(key),         DATA_INT32(1000)                           },
				{ HK(size),        DATA_SIZET(10)                             },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	/* create backend */
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	usleep(1000);
	
	ssize_t  ret;
	char     test[10], str[]="testtestm";
	
	// create new
	request_t  r_create[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_CREATE) },
		{ HK(size),       DATA_SIZET(sizeof(str))        },
		{ HK(buffer),     DATA_RAW(str, sizeof(str))     },
		hash_end
	};
	ret = backend_query(backend, r_create);
		fail_unless(ret >= 0, "create failed");	
	
	request_t  r_read[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_READ)   },
		{ HK(offset),     DATA_OFFT(0)                   },
		{ HK(size),       DATA_SIZET(sizeof(test))       },
		{ HK(buffer),     DATA_RAW(test, sizeof(test))   },
		hash_end
	};
	ret = backend_query(backend, r_read);
		fail_unless(ret > 0, "read failed");
	
	
}
END_TEST
REGISTER_TEST(core, test_backend_ipc)
