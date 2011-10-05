#include <libfrozen.h>
#include <murmur2.h>
//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

unsigned int MurmurHash2 ( data_t *data, unsigned int seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	char                   buffer[4*BUFFER_ROUNDS];
	uintmax_t              offset            = 0;
	register uintmax_t     len;
	const unsigned char   *p;
	const unsigned int     m                 = 0x5bd1e995;
	const int              r                 = 24;

	// Initialize the hash to a 'random' value
	
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if(data_query(data, &r_len) != 0)
		return 0;
	len = r_len.length;
	
	unsigned int h = seed ^ len;
	
	while(1){ 
		fastcall_read r_read = { { 5, ACTION_READ }, offset, &buffer, sizeof(buffer) };
		if(data_query(data, &r_read) != 0)
			return 0;
		
		len     = r_read.buffer_size;
		offset += len;
		
		// Mix 4 bytes at a time into the hash

		p = (const unsigned char *)buffer;

		while(len >= 4)
		{
			unsigned int k = *(unsigned int *)data;

			k *= m; 
			k ^= k >> r; 
			k *= m; 
			
			h *= m; 
			h ^= k;

			p   += 4;
			len -= 4;
		}
		if(len != 0)
			break;
	}

	// Handle the last few bytes of the input array
	switch(len)
	{
	case 3: h ^= p[2] << 16;
	case 2: h ^= p[1] << 8;
	case 1: h ^= p[0];
	        h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 
