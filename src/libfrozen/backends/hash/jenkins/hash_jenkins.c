#include "libfrozen.h"
#include "hash_jenkins.h"

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define hashsize64(n) ((uint64_t)1<<(n))
#define hashmask64(n) (hashsize64(n)-1)

#define BUFFER_ROUNDS 10

/*
   --------------------------------------------------------------------
   mix -- mix 3 32-bit values reversibly.
   For every delta with one or two bits set, and the deltas of all three
   high bits or all three low bits, whether the original value of a,b,c
   is almost all zero or is uniformly distributed,
 * If mix() is run forward or backward, at least 32 bits in a,b,c
 have at least 1/4 probability of changing.
 * If mix() is run forward, every bit of c will change between 1/3 and
 2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
 mix() was built out of 36 single-cycle latency instructions in a 
 structure that could supported 2x parallelism, like so:
 a -= b; 
 a -= c; x = (c>>13);
 b -= c; a ^= x;
 b -= a; x = (a<<8);
 c -= a; b ^= x;
 c -= b; x = (b>>13);
 ...
 Unfortunately, superscalar Pentiums and Sparcs can't take advantage 
 of that parallelism.  They've also turned some of those single-cycle
 latency instructions into multi-cycle latency instructions.  Still,
 this is the fastest good hash I could find.  There were about 2^^68
 to choose from.  I only looked at a billion or so.
 --------------------------------------------------------------------
 */
#define mix(a,b,c) \
{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8); \
	c -= a; c -= b; c ^= (b>>13); \
	a -= b; a -= c; a ^= (c>>12);  \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5); \
	a -= b; a -= c; a ^= (c>>3);  \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
}

#define mix64(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>43); \
  b -= c; b -= a; b ^= (a<<9); \
  c -= a; c -= b; c ^= (b>>8); \
  a -= b; a -= c; a ^= (c>>38); \
  b -= c; b -= a; b ^= (a<<23); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>35); \
  b -= c; b -= a; b ^= (a<<49); \
  c -= a; c -= b; c ^= (b>>11); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<18); \
  c -= a; c -= b; c ^= (b>>22); \
}

/*
   --------------------------------------------------------------------
   hash() -- hash a variable-length key into a 32-bit value
k       : the key (the unaligned variable-length array of bytes)
len     : the length of the key, counting by bytes
initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Every 1-bit and 2-bit delta achieves avalanche.
About 6*len+35 instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (uint8_t **)k, do it like this:
for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
 */

uint32_t     jenkins32_hash(uint32_t seed, data_t *data){
	register uint32_t len;
	register uint32_t a,b,c;
	uint32_t          keylen;
	uintmax_t         offset = 0;
	char              buffer[12*BUFFER_ROUNDS];
	char             *k;

	/* Set up the internal state */
	a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
	c = seed;   /* the previous hash value - seed in our case */
	
	while(1){ 
		fastcall_read r_read = { { 5, ACTION_READ }, offset, &buffer, sizeof(buffer) };
		if(data_query(data, &r_read) != 0)
			return c;
		
		k       = buffer;
		len     = r_read.buffer_size;
		offset += len;
		keylen += len;
		
		/*---------------------------------------- handle most of the key */
		while (len >= 12)
		{
			a += ((uint32_t)k[0] +((uint32_t)k[1]<<8) +((uint32_t)k[2]<<16) +((uint32_t)k[3]<<24));
			b += ((uint32_t)k[4] +((uint32_t)k[5]<<8) +((uint32_t)k[6]<<16) +((uint32_t)k[7]<<24));
			c += ((uint32_t)k[8] +((uint32_t)k[9]<<8) +((uint32_t)k[10]<<16)+((uint32_t)k[11]<<24));
			mix(a,b,c);
			k   += 12;
			len -= 12;
		}
		if(len != 0)    // last cycle
			break;
	}
	
	/*------------------------------------- handle the last 11 bytes */
	c  += keylen;
	switch(len)              /* all the case statements fall through */
	{
		case 11: 
			c +=((uint32_t)k[10]<<24);
		case 10: 
			c +=((uint32_t)k[9]<<16);
		case 9 : 
			c +=((uint32_t)k[8]<<8);
			/* the first byte of c is reserved for the length */
		case 8 : 
			b +=((uint32_t)k[7]<<24);
		case 7 : 
			b +=((uint32_t)k[6]<<16);
		case 6 : 
			b +=((uint32_t)k[5]<<8);
		case 5 :
			b +=(uint8_t) k[4];
		case 4 : 
			a +=((uint32_t)k[3]<<24);
		case 3 : 
			a +=((uint32_t)k[2]<<16);
		case 2 : 
			a +=((uint32_t)k[1]<<8);
		case 1 : 
			a +=(uint8_t)k[0];
			/* case 0: nothing left to add */
	}
	mix(a,b,c);
	
	return c;
}

uint64_t     jenkins64_hash(uint64_t seed, data_t *data){
	register uint64_t a, b, c, len;
	uint64_t          keylen;
	uintmax_t         offset = 0;
	char              buffer[24*BUFFER_ROUNDS];
	char             *k;

	/* Set up the internal state */
	a = b = seed;                         /* the previous hash value */
	c = 0x9e3779b97f4a7c13LL; /* the golden ratio; an arbitrary value */
	
	while(1){ 
		fastcall_read r_read = { { 5, ACTION_READ }, offset, &buffer, sizeof(buffer) };
		if(data_query(data, &r_read) != 0)
			return c;
		
		k       = buffer;
		len     = r_read.buffer_size;
		offset += len;
		keylen += len;

		/*---------------------------------------- handle most of the key */
		while (len >= 24){
			a += (k[0]        +((uint64_t)k[ 1]<< 8)+((uint64_t)k[ 2]<<16)+((uint64_t)k[ 3]<<24)
			+((uint64_t)k[4 ]<<32)+((uint64_t)k[ 5]<<40)+((uint64_t)k[ 6]<<48)+((uint64_t)k[ 7]<<56));
			b += (k[8]        +((uint64_t)k[ 9]<< 8)+((uint64_t)k[10]<<16)+((uint64_t)k[11]<<24)
			+((uint64_t)k[12]<<32)+((uint64_t)k[13]<<40)+((uint64_t)k[14]<<48)+((uint64_t)k[15]<<56));
			c += (k[16]       +((uint64_t)k[17]<< 8)+((uint64_t)k[18]<<16)+((uint64_t)k[19]<<24)
			+((uint64_t)k[20]<<32)+((uint64_t)k[21]<<40)+((uint64_t)k[22]<<48)+((uint64_t)k[23]<<56));
			mix64(a,b,c);
			k += 24; len -= 24;
		}
		if(len != 0)    // last cycle
			break;
	}

	/*------------------------------------- handle the last 23 bytes */
	c += keylen;
	switch(len){             /* all the case statements fall through */
		case 23: c+=((uint64_t)k[22]<<56);
		case 22: c+=((uint64_t)k[21]<<48);
		case 21: c+=((uint64_t)k[20]<<40);
		case 20: c+=((uint64_t)k[19]<<32);
		case 19: c+=((uint64_t)k[18]<<24);
		case 18: c+=((uint64_t)k[17]<<16);
		case 17: c+=((uint64_t)k[16]<<8);
		/* the first byte of c is reserved for the length */
		case 16: b+=((uint64_t)k[15]<<56);
		case 15: b+=((uint64_t)k[14]<<48);
		case 14: b+=((uint64_t)k[13]<<40);
		case 13: b+=((uint64_t)k[12]<<32);
		case 12: b+=((uint64_t)k[11]<<24);
		case 11: b+=((uint64_t)k[10]<<16);
		case 10: b+=((uint64_t)k[ 9]<<8);
		case  9: b+=((uint64_t)k[ 8]);
		case  8: a+=((uint64_t)k[ 7]<<56);
		case  7: a+=((uint64_t)k[ 6]<<48);
		case  6: a+=((uint64_t)k[ 5]<<40);
		case  5: a+=((uint64_t)k[ 4]<<32);
		case  4: a+=((uint64_t)k[ 3]<<24);
		case  3: a+=((uint64_t)k[ 2]<<16);
		case  2: a+=((uint64_t)k[ 1]<<8);
		case  1: a+=((uint64_t)k[ 0]);
		/* case 0: nothing left to add */
	}
	mix64(a,b,c);
	
	return c;
}

