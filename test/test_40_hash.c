
START_TEST (test_hash){
	hash_t  hash[] = {
		{ TYPE_STRING, "key1", "value1" },
		{ TYPE_STRING, "key2", "value2" },
		{ TYPE_STRING, "key3", "value3" },
		hash_null,
		hash_null,
		hash_null,
		hash_null,
		hash_end
	};
	
	hash_t *key1, *key2, *key3, *key4;
	
	hash_add_value(hash, TYPE_STRING, "key4", "value4");
	
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
}
END_TEST
REGISTER_TEST(core, test_hash)

