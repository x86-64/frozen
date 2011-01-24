START_TEST (test_structs){
	struct_t structure[] = {
		{ HK(key1), DATA_INT32(0)   },
		{ HK(key2), DATA_OFFT(0)    },
		{ HK(key3), DATA_INT32(0)   },
		{ HK(key4), DATA_STRING("") },
		hash_end
	};
	
	request_t values[] = {
		{ HK(key4), DATA_STRING("hello") },
		{ HK(key1), DATA_INT32(100)      },
		{ HK(key2), DATA_OFFT(10)        },
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
	
	ret = struct_pack(structure, values, &test_data, NULL);
		fail_unless(ret > 0,                                "struct_pack failed");
		fail_unless(memcmp(test, orig, sizeof(orig)) == 0,  "struct_pack data failed");
	
	DT_INT32  test1;
	DT_OFFT   test2;
	DT_STRING test3;
	
	request_t query[] = {
		{ HK(key4), DATA_STRING("") },
		{ HK(key1), DATA_INT32(0)   },
		{ HK(key2), DATA_OFFT(0)    },
		hash_end
	};
	
	ret = struct_unpack(structure, query, &test_data, NULL);
		fail_unless(ret > 0,                                "struct_unpack failed");
	
	hash_copy_data(ret, TYPE_INT32,  test1, query, HK(key1));
		fail_unless(ret == 0 && test1 == 100,               "struct_unpack data 1 failed\n");
	hash_copy_data(ret, TYPE_OFFT,   test2, query, HK(key2));
		fail_unless(ret == 0 && test2 == 10,                "struct_unpack data 2 failed\n");
	hash_copy_data(ret, TYPE_STRING, test3, query, HK(key4));
		fail_unless(ret == 0 && strcmp(test3,"hello") == 0, "struct_unpack data 3 failed\n");
	
}
END_TEST
REGISTER_TEST(core, test_structs)

