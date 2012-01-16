
START_TEST (test_machines){
	machine_t *machine, *machinef;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                     },
                        { HK(filename),    DATA_STRING("data_machine_file.dat")    },
                        hash_end
                )},
                hash_end
	};
	
	/* re-run with good parameters */
	machine = machine_new(settings);
		fail_unless(machine != NULL, "machine creation failed");
	
	request_t r_fork[] = {
		hash_end
	};
	
	machinef = machine_fork(machine, r_fork);
		fail_unless(machinef != NULL, "machine fork creation failed");
	
	machine_destroy(machinef);
	machine_destroy(machine);
}
END_TEST
REGISTER_TEST(core, test_machines)

