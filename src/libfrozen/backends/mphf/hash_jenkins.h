#ifndef JENKINS_HASH_H
#define JENKINS_HASH_H

uint32_t     jenkins32_hash(uint32_t seed, char *k, size_t keylen, uint32_t hashes[], size_t nhashes);

#endif
