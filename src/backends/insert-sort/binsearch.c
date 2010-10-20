// TODO rewrite using data_* functions

ssize_t             sorts_binsearch_find (sorts_user_data *data, buffer_t *buffer1, buffer_t *key_out){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	buffer_t        buffer_backend, buffer_count;
	
	range_start = range_end = 0;
	
	hash_t  req_count[] = {
		{ "action", DATA_INT32(ACTION_CRWD_COUNT) },
		hash_end
	};
	buffer_init_from_bare(&buffer_count, &range_end, sizeof(range_end));
	
	ret = chain_next_query(data->chain, req_count, &buffer_count);
	if(ret <= 0 || range_start == range_end){
		buffer_write(key_out, 0, &range_end, sizeof(range_end));
		ret = KEY_NOT_FOUND;
		goto cleanup2;
	}
	
	backend_buffer_io_init(&buffer_backend, (void *)data->chain, 0);
	
	/* check last element of list to fast-up incremental inserts */
	if( (ret = data_buffer_cmp(&data->cmp_ctx, buffer1, 0, &buffer_backend, range_end - 1)) >= 0){
		buffer_write(key_out, 0, &range_end, sizeof(range_end));
		ret = (ret == 0) ? KEY_FOUND : KEY_NOT_FOUND;
		goto cleanup1;
	}
	
	while(range_start + 1 < range_end){
		current = range_start + ((range_end - range_start) / 2);
		
		ret = data_buffer_cmp(&data->cmp_ctx, buffer1, 0, &buffer_backend, current);
		
		/*
		printf("range: %x-%x curr: %x ret: %x\n",
			(unsigned int)range_start, (unsigned int)range_end,
			(unsigned int)current, (unsigned int)ret
		);
		*/
		
		if(ret == 0){
			buffer_write(key_out, 0, &current, sizeof(current));
			ret = KEY_FOUND;
			goto cleanup1;
		}else if(ret < 0){
			range_end   = current;
		}else{
			range_start = current;
		}
	}
	if(data_buffer_cmp(&data->cmp_ctx, buffer1, 0, &buffer_backend, range_start) <= 0)
		current = range_start;
	else
		current = range_end;
	
	buffer_write(key_out, 0, &current, sizeof(current));
	ret = KEY_NOT_FOUND;
	
cleanup1:
	buffer_destroy(&buffer_backend);
cleanup2:
	buffer_destroy(&buffer_count);
	return ret;
}

REGISTER(`{ .name = "binsearch", .func_find = &sorts_binsearch_find }')
