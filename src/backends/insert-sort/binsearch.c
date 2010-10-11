
ssize_t             sorts_binsearch_find (sorts_user_data *data, buffer_t *buffer1, buffer_t *key_out){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	buffer_t        buffer_backend;
	
	range_start = range_end = 0;
	if(
		fc_crwd_chain(&data->fc_table, data->chain, ACTION_CRWD_COUNT, &range_end, sizeof(range_end)) <= 0 || 
		range_start == range_end
	){
		buffer_write(key_out, 0, &range_end, sizeof(range_end));
		return KEY_NOT_FOUND;
	}
	
	backend_buffer_io_init(&buffer_backend, (void *)data->chain, 0);
	
	/* check last element of list to fast-up incremental inserts */
	if( (ret = data_buffer_cmp(&data->cmp_ctx, buffer1, 0, &buffer_backend, range_end - 1)) >= 0){
		buffer_write(key_out, 0, &range_end, sizeof(range_end));
		return (ret == 0) ? KEY_FOUND : KEY_NOT_FOUND;
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
			goto cleanup;
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
	
cleanup:
	buffer_destroy(&buffer_backend);
	return ret;
}

REGISTER(`{ .name = "binsearch", .func_find = &sorts_binsearch_find }')
