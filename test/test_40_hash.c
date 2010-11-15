
START_TEST (test_hash){
	hash_t  hash[] = {
		{ "key1", DATA_STRING("value1") },
		{ "key2", DATA_STRING("value2") },
		{ "key3", DATA_STRING("value3") },
		{ "key4", DATA_STRING("value4") },
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_end
	};
	
	hash_t *key1, *key2, *key3, *key4, *key2_n;
	
	fail_unless(hash_find(hash, "key_no") == NULL, "hash found invalid entry");
	fail_unless( (key1 = hash_find(hash, "key1"))   != NULL, "hash not found existing entry");
	fail_unless( (key2 = hash_find(hash, "key2"))   != NULL, "hash not found existing entry");
	fail_unless( (key3 = hash_find(hash, "key3"))   != NULL, "hash not found existing entry");
	fail_unless( (key4 = hash_find(hash, "key4"))   != NULL, "hash not found existing entry");
	
	fail_unless(strcmp(hash_get_value_ptr(key1), "value1") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key2), "value2") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key3), "value3") == 0, "hash found wrong entry");
	fail_unless(strcmp(hash_get_value_ptr(key4), "value4") == 0, "hash found wrong entry");
	
	hash_delete(hash, "key2");
	
	fail_unless( (key2 = hash_find(hash, "key2")) == NULL, "necrozz attack!!");
	
	hash_t hash2[] = {
		{ "key2", DATA_STRING("value_next_2") },
		hash_next(hash)
	};
	fail_unless( (key2_n = hash_find(hash2, "key2"))   != NULL, "assigned hash not found existing entry");
	fail_unless(strcmp(hash_get_value_ptr(key2_n), "value_next_2") == 0,             "assigned hash found wrong entry");
	fail_unless( (key3 = hash_find(hash, "key3"))   != NULL,    "assigned hash not found existing entry");
	fail_unless(strcmp(hash_get_value_ptr(key3), "value3") == 0,                   "assigned hash found wrong entry");
}
END_TEST
REGISTER_TEST(core, test_hash)

