#ifndef MPHF_CHM_IMP_H
#define MPHF_CHM_IMP_H

ssize_t  mphf_chm_imp_new         (mphf_t *mphf);
ssize_t  mphf_chm_imp_build_start (mphf_t *mphf);
ssize_t  mphf_chm_imp_build_end   (mphf_t *mphf);
ssize_t  mphf_chm_imp_insert      (mphf_t *mphf, char *key, size_t key_len, uint64_t  value);
ssize_t  mphf_chm_imp_query       (mphf_t *mphf, char *key, size_t key_len, uint64_t *value);
ssize_t  mphf_chm_imp_delete      (mphf_t *mphf, char *key, size_t key_len);

#endif
