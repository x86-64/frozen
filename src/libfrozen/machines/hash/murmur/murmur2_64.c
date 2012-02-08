#include <libfrozen.h>
#include <murmur2.h>
//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.

//typedef unsigned __int64 uint64_t;

// 64-bit hash for 64-bit platforms

uint64_t MurmurHash64A ( data_t *data, unsigned int seed ){
	fastcall_length r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(clean) };
	if( data_query(data, &r_len) != 0 )
		return 0;
	
	char                   buffer[8*BUFFER_ROUNDS];
	uintmax_t              offset            = 0;
	const uint64_t        *p;
	const uint64_t        *p_end;
	const uint64_t         m                 = 0xc6a4a7935bd1e995;
	const int              r                 = 47;
	uint64_t               h                 = seed ^ (r_len.length * m);

	while(1){ 
		fastcall_read r_read = { { 5, ACTION_READ }, offset, &buffer, sizeof(buffer) };
		if(data_query(data, &r_read) != 0)
			return 0;
		
		p       = (const uint64_t *)buffer;
		p_end   = p + (r_read.buffer_size / 8);
		offset += r_read.buffer_size;
	
		while(p != p_end)
		{
			uint64_t k = *p++;

			k *= m; 
			k ^= k >> r; 
			k *= m; 
			
			h ^= k;
			h *= m; 
		}
		if(r_read.buffer_size != sizeof(buffer))
			break;
	}
	
	const unsigned char * p2 = (const unsigned char*)p;

	switch(r_len.length & 7)
	{
	case 7: h ^= (uint64_t)p2[6] << 48;
	case 6: h ^= (uint64_t)p2[5] << 40;
	case 5: h ^= (uint64_t)p2[4] << 32;
	case 4: h ^= (uint64_t)p2[3] << 24;
	case 3: h ^= (uint64_t)p2[2] << 16;
	case 2: h ^= (uint64_t)p2[1] << 8;
	case 1: h ^= (uint64_t)p2[0];
	        h *= m;
	};
 
	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
} 

