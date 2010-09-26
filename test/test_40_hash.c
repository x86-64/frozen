
START_TEST (test_hash){
	char            data1_buff[] = "\x11\x22\x00\x00";
	char            data2_buff[] = "\x33\x44\x00\x00";
	data_t         *data1 = (data_t *)&data1_buff;
	data_t         *data2 = (data_t *)&data2_buff;
	data_t         *test_data;
	size_t          test_size;
	int             ret;
	
	hash_t *hash = hash_new();
		fail_unless(hash != NULL, "hash_new failed");
	
	ret = hash_set(hash, "test1", TYPE_INT32, data1);
		fail_unless(ret == 0,     "hash_set 1 failed");
	ret = hash_set(hash, "test2", TYPE_INT32, data2);
		fail_unless(ret == 0,     "hash_set 2 failed");
	
	ret = hash_get(hash, "test1", TYPE_INT32, &test_data, &test_size);
		fail_unless(ret == 0 && test_size == 4,               "hash_get 1 failed");
		fail_unless(memcmp(test_data, data1, test_size) == 0, "hash_get 1 data failed");
	ret = hash_get(hash, "test2", TYPE_INT32, &test_data, &test_size);
		fail_unless(ret == 0 && test_size == 4,               "hash_get 2 failed");
		fail_unless(memcmp(test_data, data2, test_size) == 0, "hash_get 2 data failed");
	
	hash_free(hash);
}
END_TEST
REGISTER_TEST(core, test_hash)

