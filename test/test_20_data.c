
START_TEST (test_data){
	fail_unless( data_type_from_string("void") == TYPE_VOID, "data types invalid");
	
	// test data_convert
	data_t  d_from1 = DATA_STRING("12345");
	data_t  d_from2 = DATA_STRING("12346");
	data_t  d_from3 = DATA_STRING("12347");
	data_t  d_to   = DATA_INT32(0);
	ssize_t retval;
	
	data_convert_to_alloc(retval, TYPE_INT32, &d_to, &d_from1, NULL);
	if(retval == 0){
		fail_unless( *(unsigned int *)d_to.data_ptr == 12345, "data_convert_to_alloc data failed");
		
		data_free(&d_to);
	}else{
		fail("data_convert call failed");
	}
	
	data_convert_to_local(retval, TYPE_INT32, &d_to, &d_from2, NULL);
	if(retval == 0){
		fail_unless( *(unsigned int *)d_to.data_ptr == 12346, "data_convert_to_local data failed");
	}else{
		fail("data_convert call failed");
	}

	DT_INT32 test_dt;
	data_convert_to_dt(retval, TYPE_INT32, test_dt, &d_from3, NULL);
	if(retval == 0){
		fail_unless( test_dt == 12347, "data_convert_to_dt data failed");
	}else{
		fail("data_convert call failed");
	}
}
END_TEST
REGISTER_TEST(core, test_data)

