
START_TEST(test_backend_ipc){
	backend_t  *backend;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                        },
                        { HK(filename),    DATA_STRING("data_backend_ipc.dat")        },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("ipc")                         },
                        { HK(type),        DATA_STRING("shmem")                       },
                        { HK(role),        DATA_STRING("server")                      },
                        { HK(key),         DATA_UINT32T(1000)                         },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("ipc")                         },
                        { HK(type),        DATA_STRING("shmem")                       },
                        { HK(role),        DATA_STRING("client")                      },
                        { HK(key),         DATA_UINT32T(1000)                         },
                        hash_end
                )},
                hash_end
	};
	
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	usleep(1000);
	
	ssize_t  ret = -1;
	char     test[10] = {0}, str[]="testtestm";
	
	// create new
	request_t  r_create[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CREATE) },
		{ HK(size),       DATA_SIZET(sizeof(str))          },
		{ HK(buffer),     DATA_RAW(str, sizeof(str))       },
		{ HK(ret),        DATA_PTR_SIZET(&ret)             },
		hash_end
	};
	backend_query(backend, r_create);
		fail_unless(ret >= 0, "create failed");	
	
        ret = -1;
	request_t  r_read[] = {
		{ HK(action),     DATA_UINT32T(ACTION_READ) },
		{ HK(offset),     DATA_OFFT(0)                   },
		{ HK(size),       DATA_SIZET(sizeof(test))       },
		{ HK(buffer),     DATA_RAW(test, sizeof(test))   },
		{ HK(ret),        DATA_PTR_SIZET(&ret)           },
                hash_end
	};
	backend_query(backend, r_read);
		fail_unless(ret > 0,                "read failed");
		fail_unless(strcmp(test, str) == 0, "read data failed");
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_ipc)

/*
#define NTESTS 100000

void timersub (x, y, result)
        struct timeval *result, *x, *y; 
{
    
        if (x->tv_usec < y->tv_usec) {
                int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
                y->tv_usec -= 1000000 * nsec;
                y->tv_sec += nsec;
        }   
        if (x->tv_usec - y->tv_usec > 1000000) {
                int nsec = (x->tv_usec - y->tv_usec) / 1000000;
                y->tv_usec += 1000000 * nsec;
                y->tv_sec -= nsec;
        }   
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;
}

START_TEST(test_backend_ipc_speed){
	backend_t  *backend;
	
	hash_t  settings[] = {
		{ HK(backends), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("null-proxy")                  },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("ipc")                         },
				{ HK(type),        DATA_STRING("shmem")                       },
				{ HK(role),        DATA_STRING("server")                      },
				{ HK(key),         DATA_UINT32T(1001)                           },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("ipc")                         },
				{ HK(type),        DATA_STRING("shmem")                       },
				{ HK(role),        DATA_STRING("client")                      },
				{ HK(key),         DATA_UINT32T(1001)                           },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	usleep(1000);
	
	int             i, failed = 0;
	struct timeval  start, end, res;
	char            str[]="testtestm";
	
	// create new
	request_t  r_create[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CREATE) },
		{ HK(size),       DATA_SIZET(0x0000BEEF)         },
		{ HK(buffer),     DATA_RAW(str, sizeof(str))     },
		hash_end
	};
	
	gettimeofday(&start, NULL);
	for(i=0; i<NTESTS; i++){
		if(backend_query(backend, r_create) != 0x0000BEEF)
			failed++;
	}
	gettimeofday(&end, NULL);
	timersub(&end, &start, &res);
	printf("ipc query: %u iters took %d.%.6d sec; speed: %7lu iters/s, failed: %d\n",
		NTESTS,
		(int)res.tv_sec,
		(int)res.tv_usec,
		( NTESTS / ((res.tv_sec * 1000 + res.tv_usec / 1000)+1) * 1000 ),
		failed
	);
	
	backend_destroy(backend);
}
END_TEST
REEEGISTER_TEST(core, test_backend_ipc_speed)
*/
