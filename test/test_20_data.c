
START_TEST (test_data){
	fail_unless( data_type_from_string("void") == TYPE_VOID, "data types invalid");
	
	// test data_convert
	data_t d_from = DATA_STRING("12345");
	data_t d_to   = DATA_INT32(0);
	
	if(data_convert(&d_to, NULL, &d_from, NULL) == 0){
		fail_unless( *(unsigned int *)d_to.data_ptr == 12345, "data_convert data failed");
		
		data_free(&d_to);
	}else{
		fail("data_convert call failed");
	}
}
END_TEST
REGISTER_TEST(core, test_data)

