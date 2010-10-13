
START_TEST (test_data){
	fail_unless( data_type_from_string("void") == TYPE_VOID, "data types invalid");
}
END_TEST
REGISTER_TEST(core, test_data)

