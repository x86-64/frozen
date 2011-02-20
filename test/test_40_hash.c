
START_TEST (test_hash){
	hash_t  hash[] = {
		{ 1, DATA_STRING("value1") },
		{ 2, DATA_STRING("value2") },
		{ 3, DATA_STRING("value3") },
		{ 4, DATA_STRING("value4") },
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_end
	};
	
	hash_t *key1, *key2, *key3, *key4, *key2_n;
	
	fail_unless(hash_find(hash, 7777) == NULL, "hash found invalid entry");
	fail_unless( (key1 = hash_find(hash, 1))   != NULL, "hash not found existing entry");
	fail_unless( (key2 = hash_find(hash, 2))   != NULL, "hash not found existing entry");
	fail_unless( (key3 = hash_find(hash, 3))   != NULL, "hash not found existing entry");
	fail_unless( (key4 = hash_find(hash, 4))   != NULL, "hash not found existing entry");
	
	fail_unless(strcmp(hash_get_value_ptr(key1), "value1") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key2), "value2") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key3), "value3") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key4), "value4") == 0, "hash found wrong entry");
	
	hash_delete(hash, 2);
	
	fail_unless( (key2 = hash_find(hash, 2)) == NULL, "necrozz attack!!");
	
	hash_t hash2[] = {
		{ 2, DATA_STRING("value_next_2") },
		hash_next(hash)
	};
	fail_unless( (key2_n = hash_find(hash2, 2))   != NULL, "assigned hash not found existing entry");
	fail_unless(strcmp(hash_get_value_ptr(key2_n), "value_next_2") == 0,             "assigned hash found wrong entry");
	fail_unless( (key3 = hash_find(hash, 3))   != NULL,    "assigned hash not found existing entry");
	fail_unless(strcmp(hash_get_value_ptr(key3), "value3") == 0,                   "assigned hash found wrong entry");
	
	hash_t   *hash_from;
	buffer_t  buffer;
	ssize_t   ret;
	
	ret = hash_to_buffer(hash, &buffer);
		fail_unless(ret == 0, "hash_to_buffer failed");
	
	ret = hash_from_buffer(&hash_from, &buffer);
		fail_unless(ret == 0, "hash_from_buffer failed");
	
	// TODO validate
	
	buffer_destroy(&buffer);
}
END_TEST
REGISTER_TEST(core, test_hash)

