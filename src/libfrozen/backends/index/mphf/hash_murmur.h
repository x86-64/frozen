#ifndef HASH_MURMUR_H
#define HASH_MURMUR_H

uint64_t murmur64_hash(uint64_t seed, char *key, size_t len, uint64_t hashes[], size_t nhashes);

#endif
