
START_TEST (test_structs){
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ "name", DATA_STRING("data1") },
			{ "type", DATA_STRING("int32") },
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "name", DATA_STRING("data2") },
			{ "type", DATA_STRING("int32") },
			hash_end
		)},
		hash_end
	};
	
	struct_t *structure = struct_new(settings);
		fail_unless(structure != NULL,             "structure create failed");
	member_t *m_d1 = struct_get_member_by_name(structure, "data1");
	member_t *m_d2 = struct_get_member_by_name(structure, "data2");
		fail_unless(m_d1 != NULL && m_d2 != NULL,  "member search failed");
		fail_unless(m_d1 != m_d2,                  "member search invalid");
		fail_unless(m_d1->data_type == TYPE_INT32, "member size invalid");
	
	size_t size = struct_get_size(structure);
		fail_unless(size == 8,                     "structure size invalid");
	
	/*
	chunk_t     chunk;
	buffer_t   *buffer  = buffer_from_data("\x11\x22\x00\x00\x33\x44\x00\x00", 8);
	chunk_t    *b_chunk = buffer_get_first_chunk(buffer);
	
	ret = struct_get_member_value(structure, buffer, 0, &chunk);
		fail_unless(ret == 0 && chunk.size == 4,                                       "structure_get_member_value 1 failed");
		fail_unless(memcmp(chunk.bare_ptr, "\x11\x22\x00\x00", chunk.size) == 0,            "structure_get_member_value 1 data failed");
	ret = struct_get_member_value(structure, buffer, 1, &chunk);
		fail_unless(ret == 0 && chunk.size == 4,                                       "structure_get_member_value 2 failed");
		fail_unless(memcmp(chunk.bare_ptr, "\x33\x44\x00\x00", chunk.size) == 0,            "structure_get_member_value 2 data failed");



	buffer_free(buffer);
	*/
	
	struct_free(structure);
}
END_TEST
REGISTER_TEST(core, test_structs)

