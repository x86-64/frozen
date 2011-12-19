START_TEST (test_structs){
	hash_t structure[] = {
		{ HK(key1), DATA_HASHT(hash_end) },
		{ HK(key2), DATA_HASHT(hash_end) },
		{ HK(key3), DATA_HASHT(
			{ HK(default), DATA_UINT32T(0) }, 
			hash_end
		)},
		{ HK(key4), DATA_HASHT(hash_end) },
		hash_end
	};
	
	request_t values[] = {
		{ HK(key4), DATA_STRING("hello") },
		{ HK(key1), DATA_UINT32T(100)    },
		{ HK(key2), DATA_OFFT(10)        },
		{ HK(key3), DATA_UINT32T(0)      },
		hash_end
	};
	
	ssize_t ret;
	char    test[100] = {0};
	char    orig[] =
		"\x64\x00\x00\x00"
		"\x0A\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00"
		"hello\x00";
	data_t  test_data = DATA_RAW(test, 100);
	
	ret = struct_pack(structure, values, &test_data);
		fail_unless(ret > 0,                                "struct_pack failed");
		fail_unless(memcmp(test, orig, sizeof(orig)) == 0,  "struct_pack data failed");
	
	uint32_t  test1;
	off_t     test2;
	char     *test3 = malloc(100);
	memset(test3, 0x41, 99);
	test3[99] = 0;
	
	request_t query[] = {
		{ HK(key4), DATA_STRING(test3) },
		{ HK(key1), DATA_UINT32T(0)   },
		{ HK(key2), DATA_OFFT(0)    },
		hash_end
	};
	
	ret = struct_unpack(structure, query, &test_data);
		fail_unless(ret > 0,                                "struct_unpack failed");
	
	hash_data_copy(ret, TYPE_UINT32T,  test1, query, HK(key1));
		fail_unless(ret == 0 && test1 == 100,               "struct_unpack data 1 failed\n");
	hash_data_copy(ret, TYPE_OFFT,   test2, query, HK(key2));
		fail_unless(ret == 0 && test2 == 10,                "struct_unpack data 2 failed\n");
	hash_data_copy(ret, TYPE_STRINGT, test3, query, HK(key4));
		fail_unless(ret == 0 && strcmp(test3,"hello") == 0, "struct_unpack data 3 failed\n");
	
	free(test3);
}
END_TEST
REGISTER_TEST(core, test_structs)

