
START_TEST (test_machine_cache_app){
	machine_t *machine;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_machine_cache_app.dat")    },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("cache-append")                  },
                        { HK(size),        DATA_SIZET(100)                              },
                        hash_end
                )},
                hash_end
	};
	
	/* create machine */
	machine = machine_new(settings);
		fail_unless(machine != NULL, "machine creation failed");
	
	/* test read/writes */
	ssize_t         ret;
	size_t          i;
        char            buf[30] = {0};
        

        for(i=0; i<=10; i++){
                request_t       request[] = {
                        { HK(action),  DATA_UINT32T(ACTION_CREATE)  },
                        { HK(buffer),  DATA_RAW(buf, 30)               },
                        { HK(size),    DATA_SIZET(30)                  },
                        hash_end
                };
                
                ret = machine_query(machine, request);
                        fail_unless(ret >= 0, "write failed");
        }
	
	machine_destroy(machine);
}
END_TEST
REGISTER_TEST(core, test_machine_cache_app)


