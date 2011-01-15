
ssize_t             sorts_binsearch_find (sorts_userdata *data, data_t *buffer1, data_ctx_t *buffer1_ctx, data_t *key_out, data_ctx_t *key_out_ctx){
	ssize_t         ret;
	off_t           range_start, range_end, current;
	data_t          d_current = DATA_PTR_OFFT(&current);
	
	range_start = range_end = 0;
	
	hash_t  req_count[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_PTR_OFFT(&range_end)     },
		hash_end
	};
	ret = chain_next_query(data->chain, req_count);
	if(ret <= 0 || range_start == range_end){
		current = range_end;
		
		ret = KEY_NOT_FOUND;
		goto exit;
	}
	
	data_t  backend = DATA_CHAINT(data->chain);
	hash_t  backend_ctx[] = {
		{ HK(offset),  DATA_PTR_OFFT(&current)  },
		hash_end
	};
	
	/* check last element of list to fast-up incremental inserts */
	current = range_end - 1;
	if( (ret = data_cmp(buffer1, buffer1_ctx, &backend, backend_ctx)) >= 0){
		current = range_end;
		ret = (ret == 0) ? KEY_FOUND : KEY_NOT_FOUND;
		
		goto exit;
	}
	
	while(range_start + 1 < range_end){
		current = range_start + ((range_end - range_start) / 2);
		
		ret = data_cmp(buffer1, buffer1_ctx, &backend, backend_ctx);
		
		/*
		printf("range: %x-%x curr: %x ret: %x\n",
			(unsigned int)range_start, (unsigned int)range_end,
			(unsigned int)current, (unsigned int)ret
		);
		*/
		
		if(ret == 0){
			ret = KEY_FOUND;
			goto exit;
		}else if(ret < 0){
			range_end   = current;
		}else{
			range_start = current;
		}
	}
	current = range_start;
	if(data_cmp(buffer1, buffer1_ctx, &backend, backend_ctx) <= 0)
		current = range_start;
	else
		current = range_end;
	
	ret = KEY_NOT_FOUND;
exit:
	data_transfer(
		key_out,    key_out_ctx,
		&d_current, NULL
	);
	return ret;
}

REGISTER(`{ .name = "binsearch", .func_find = &sorts_binsearch_find }')
