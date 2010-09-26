
START_TEST (test_hash){
	char            data1_buff[] = "\x11\x22\x00\x00";
	char            data2_buff[] = "\x33\x44\x00\x00";
	char            data_tbuff[] = "\x00\x00\x00\x00";
	data_type       data_type;
	data_t         *data1 = (data_t *)&data1_buff;
	data_t         *data2 = (data_t *)&data2_buff;
	data_t         *data_test;
	
	hash_builder_t *builder;
	hash_t         *hash;
	size_t          hash_size;
	int             ret;
	
	builder = hash_builder_new(10);
		fail_unless(builder != NULL, "hash_builder alloc failed");
	
	ret = hash_builder_add_data(&builder, "test1", TYPE_INT32, data1); 
		fail_unless(ret == 0,        "hash_builder add_data 1 failed");	
	ret = hash_builder_add_data(&builder, "test2", TYPE_INT32, data2);
		fail_unless(ret == 0,        "hash_builder add_data 2 failed");	
	
	hash = hash_builder_get_hash(builder);
		fail_unless(hash != NULL,    "hash_builder_get_hash_ptr failed");
	hash_size = hash->size;
		fail_unless(hash_size == sizeof(hash_t) + 11*sizeof(hash_el_t) + 6 + 6 + 4 + 4, "hash_builder_get_hash_size failed");
		
	ret = hash_audit(hash, 1000);
		fail_unless(ret == 0,                                     "hash_audit failed");
	
	ret = hash_get(hash, "test1", &data_type, &data_test);
		fail_unless(ret == 0,                                     "hash_get_data 1 failed");
		fail_unless(data_cmp(data_type, data_test, data1) == 0,   "hash_get_data 1 data failed");
	ret = hash_get(hash, "test2", &data_type, &data_test);
		fail_unless(ret == 0,                                     "hash_get_data 2 failed");
		fail_unless(data_cmp(data_type, data_test, data2) == 0,   "hash_get_data 2 data failed");
	
	data_test = hash_get_typed(hash, "test1", TYPE_INT32);
		fail_unless(data_test != NULL,                            "hash_get_typed 1 failed");
		fail_unless(data_cmp(TYPE_INT32, data_test, data1) == 0,  "hash_get_typed 1 data failed");
	
	data_test = hash_get_typed(hash, "test2", TYPE_INT32);
		fail_unless(data_test != NULL,                            "hash_get_typed 2 failed");
		fail_unless(data_cmp(TYPE_INT32, data_test, data2) == 0,  "hash_get_typed 2 data failed");
	
	ret = hash_get_in_buf(hash, "test1", TYPE_INT32, &data_tbuff);
		fail_unless(ret == 0,                                     "hash_get_in_buf 1 failed");
		fail_unless(data_cmp(TYPE_INT32, &data_tbuff, data1) == 0,"hash_get_in_buf 1 data failed");
	
	ret = hash_get_in_buf(hash, "test2", TYPE_INT32, &data_tbuff);
		fail_unless(ret == 0,                                     "hash_get_in_buf 2 failed");
		fail_unless(data_cmp(TYPE_INT32, &data_tbuff, data2) == 0,"hash_get_in_buf 2 data failed");
	
	hash_builder_free(builder);
}
END_TEST
REGISTER_TEST(core, test_hash)


