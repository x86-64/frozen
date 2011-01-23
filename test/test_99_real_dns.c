
struct dns_entry {
	char          *domain;
	unsigned int   ip;
	unsigned int   timestamp;
};

START_TEST (test_real_dns){
	struct dns_entry data_array[] = {
		{ "google.ru",  0x01010101, 0xFFFF0001 },
		{ "yandex.ru",  0x02020202, 0xFFFF0002 },
		{ "bing.com",   0x03030303, 0xFFFF0003 },
		{ "hello.com",  0x04040404, 0xFFFF0004 },
		{ "example.ru", 0x05050505, 0xFFFF0005 }
	};
	
	ssize_t  ret;
	
	hash_t     *c_idx = configs_file_parse("test_99_real_dns.conf");
		fail_unless(c_idx != NULL, "chain 'real_dns' config parse failed");
	ret = backend_bulk_new(c_idx);
		fail_unless(ret == 0,      "chain 'real_dns' backends create failed");
	hash_free(c_idx);
	
	data_t     n_idx = DATA_STRING("real_dns"), n_dat = DATA_STRING("real_dns_domains");
	backend_t *b_idx = backend_find(&n_idx);
	backend_t *b_dat = backend_find(&n_dat);
	
	off_t  data_ptr;
	
	// write array to file
	int      i;
	
	for(i=0; i < sizeof(data_array) / sizeof(struct dns_entry); i++){
		request_t r_write[] = {
			{ HK(action),  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ HK(offset_out), DATA_PTR_OFFT   (&data_ptr)                           },
			
			{ HK(dns_domain), DATA_PTR_STRING_AUTO(data_array[i].domain)            },
			{ HK(dns_ip),     DATA_INT32  (data_array[i].ip)                        },
			{ HK(dns_tstamp), DATA_INT32  (data_array[i].timestamp)                 },
			hash_end
		};
		ret = backend_query(b_idx, r_write);
			fail_unless(ret > 0,    "chain 'real_dns': write array failed");
		
		//printf("writing: ret: %x, ptr: %d, str: %s\n", ret, (unsigned int)data_ptr, data_array[i].domain);
	}
	
	/*
	// check
	char data_read[1024], data_last[1024];
	
	memset(data_last, 0, 1024);
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(data_read, 0, 1024);
		
		request_t r_read[] = {
			{ HK(action), DATA_INT32(ACTION_CRWD_READ)      },
			{ HK(offset),    DATA_OFFT(i)                      },
			{ HK(buffer), DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(b_idx, r_read);
			fail_unless(ret > 0,                                "chain 'real_store_idx_str': read array failed");
			fail_unless(memcmp(data_read, data_last, 1024) > 0, "chain 'real_store_idx_str': sort array failed");
		
		//printf("reading: ret: %x, ptr: %d, str: %s\n", ret, i, data_read);
		
		memcpy(data_last, data_read, 1024);
	}
	*/
	backend_destroy(b_idx);
	backend_destroy(b_dat);
}
END_TEST
REGISTER_TEST(core, test_real_dns)
