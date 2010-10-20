
START_TEST (test_hash){
	hash_t  hash[] = {
		{ "key1", DATA_STRING("value1") },
		{ "key2", DATA_STRING("value2") },
		{ "key3", DATA_STRING("value3") },
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_end
	};
	
	hash_t *key1, *key2, *key3, *key4, *key2_n;
	
	hash_set_value(hash, TYPE_STRING, "key4", "value4", 7);
	
	fail_unless(hash_find_value(hash, "key_no") == NULL, "hash found invalid entry");
	fail_unless( (key1 = hash_find_value(hash, "key1"))   != NULL, "hash not found existing entry");
	fail_unless( (key2 = hash_find_value(hash, "key2"))   != NULL, "hash not found existing entry");
	fail_unless( (key3 = hash_find_value(hash, "key3"))   != NULL, "hash not found existing entry");
	fail_unless( (key4 = hash_find_value(hash, "key4"))   != NULL, "hash not found existing entry");
	
	fail_unless(strcmp(key1->value, "value1") == 0, "hash found wrong entry");
	fail_unless(strcmp(key2->value, "value2") == 0, "hash found wrong entry");
	fail_unless(strcmp(key3->value, "value3") == 0, "hash found wrong entry");
	fail_unless(strcmp(key4->value, "value4") == 0, "hash found wrong entry");
	
	hash_del_value(hash, "key2");
	
	fail_unless( (key2 = hash_find_value(hash, "key2")) == NULL, "necrozz attack!!");
	
	hash_t hash2[] = {
		{ "key2", DATA_STRING("value_next_2") },
		hash_next(hash)
	};
	fail_unless( (key2_n = hash_find_value(hash2, "key2"))   != NULL, "assigned hash not found existing entry");
	fail_unless(strcmp(key2_n->value, "value_next_2") == 0,             "assigned hash found wrong entry");
	fail_unless( (key3 = hash_find_value(hash, "key3"))   != NULL,    "assigned hash not found existing entry");
	fail_unless(strcmp(key3->value, "value3") == 0,                   "assigned hash found wrong entry");
}
END_TEST
REGISTER_TEST(core, test_hash)

