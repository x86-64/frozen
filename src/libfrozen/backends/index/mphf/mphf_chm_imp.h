#ifndef MPHF_CHM_IMP_H
#define MPHF_CHM_IMP_H

ssize_t  mphf_chm_imp_load        (mphf_t *mphf);
ssize_t  mphf_chm_imp_unload      (mphf_t *mphf);
ssize_t  mphf_chm_imp_fork        (mphf_t *mphf, request_t *request);
ssize_t  mphf_chm_imp_rebuild     (mphf_t *mphf);
ssize_t  mphf_chm_imp_destroy     (mphf_t *mphf);

ssize_t  mphf_chm_imp_insert      (mphf_t *mphf, uintmax_t key, uintmax_t  value);
ssize_t  mphf_chm_imp_update      (mphf_t *mphf, uintmax_t key, uintmax_t  value);
ssize_t  mphf_chm_imp_query       (mphf_t *mphf, uintmax_t key, uintmax_t *value);
ssize_t  mphf_chm_imp_delete      (mphf_t *mphf, uintmax_t key);

#endif
