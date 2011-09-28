
START_TEST (test_data){
	fail_unless( data_type_from_string("void_t") == TYPE_VOIDT, "data types invalid");
	
	// test data_convert
	ssize_t                ret;
	uint32_t               d_to;
	data_t                 d_from1 = DATA_STRING("12345");
	
	data_convert(ret, TYPE_UINT32T, d_to, &d_from1);
		fail_unless( ret  == 0,     "data_convert failed");
		fail_unless( d_to == 12345, "data_convert data failed");
}
END_TEST
REGISTER_TEST(core, test_data)

