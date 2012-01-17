
START_TEST (test_machines_two_machines){
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),        DATA_STRING("file")                     },
                        { HK(filename),     DATA_STRING("data_machine_file.dat")    },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),        DATA_STRING("null")                     },
                        hash_end
                )},
                hash_end
	};
	
	/* create machine */
	machine_t *machine = shop_new(settings);
		fail_unless(machine != NULL, "machine creation failed");
	
	/* test read/writes */
	ssize_t         ssize;
	hash_t          request[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CREATE)  },
		{ HK(size),    DATA_SIZET( 0xBEEF )         },
                hash_end
	};
	
	ssize = machine_query(machine, request);
		fail_unless(ssize == 0x0000BEEF, "machine machine incorrectly set");
		
	shop_destroy(machine);
}
END_TEST
REGISTER_TEST(core, test_machines_two_machines)

