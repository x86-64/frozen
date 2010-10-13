#include <test.h>
#include <sys/time.h>

START_TEST (test_backend_insert_sort){
	backend_t      *backend;
	crwd_fastcall   fc_table; 
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",        "file");
			setting_set_child_string(s_file, "filename",    "data_backend_insert_sort");
		setting_t *s_list = setting_new();
			setting_set_child_string(s_list, "name",        "list");
		setting_t *s_sort = setting_new();
			setting_set_child_string(s_sort, "name",        "insert-sort");
			setting_set_child_string(s_sort, "sort-engine", "binsearch");
			setting_set_child_string(s_sort, "type",        "int8");
			
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_list);
	setting_set_child_setting(settings, s_sort);
	
	if( (backend = backend_new("in_sort", settings)) == NULL){
		fail("backend creation failed");
		return;
	}
	
	fc_crwd_init(&fc_table);	
	
	char           buff;
	buffer_t      *temp;
	ssize_t        ret;
	int            i, iters = 10;
	struct timeval tv;
	
	for(i=0; i<iters; i++){
		gettimeofday(&tv, NULL);
		
		buff = (char)(0x61 + (tv.tv_usec % 26));
		ret = fc_crwd_backend(&fc_table, backend, ACTION_CRWD_WRITE, (off_t)0, (void *)&buff, (unsigned int)sizeof(buff));
			fail_unless(ret == sizeof(buff),  "chain in_sort write failed");
	}
	
	ret = fc_crwd_backend(&fc_table, backend, ACTION_CRWD_READ, (off_t)0, (void *)&temp, (unsigned int)iters);
		fail_unless(ret == iters, "chain in_sort read failed");
	
	for(i=0; i<iters-1; i++){
		if(buffer_memcmp(temp, i, temp, i+1, 1) > 0)
			fail("chain in_sort wrong inserts");
	}
	
	fc_crwd_destroy(&fc_table);
	
	backend_destroy(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_insert_sort)

