
START_TEST (test_machines){
	machine_t *shop;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                     },
                        { HK(filename),    DATA_STRING("data_machine_file.dat")    },
                        hash_end
                )},
                hash_end
	};
	
	shop = shop_new(settings);
		fail_unless(shop != NULL, "machine creation failed");
	
	shop_destroy(shop);
}
END_TEST
REGISTER_TEST(core, test_machines)

