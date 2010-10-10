
ssize_t             sorts_binsearch_find (sorts_user_data *data, buffer_t *buffer1, buffer_t *key_out){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	size_t          range_elements;
	buffer_t       *buffer2 = NULL;
	
	range_start = 0;
	if( fc_crwd_chain(&data->fc_table, ACTION_CRWD_COUNT, &range_start, sizeof(range_start)) <= 0)
		return -EINVAL;
	
	if(range_start == range_end){
		buffer_write(key_out, 0, &range_end, sizeof(range_end));
		return KEY_NOT_FOUND;
	}
	
	while(1){
		range_elements = (range_end - range_start);
		current        = range_start + (range_elements / 2);
		
		ret = data_buffer_cmp(&data->cmp_ctx, buffer1, 0, buffer2, 0);
		if(ret == 0){
			buffer_write(key_out, 0, &current, sizeof(current));
			return KEY_FOUND;
		}else if(ret < 0){
			range_end   = current;
		}else{
			range_start = current;
		}
		
		if(range_elements <= 1){
			buffer_write(key_out, 0, &current, sizeof(current));
			return KEY_NOT_FOUND;
		}
	}
}

REGISTER(`{ .name = "binsearch", .func_find = &sorts_binsearch_find }')
