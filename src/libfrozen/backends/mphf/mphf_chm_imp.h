#ifndef MPHF_CHM_IMP_H
#define MPHF_CHM_IMP_H

ssize_t  mphf_chm_imp_new         (mphf_t *mphf);
ssize_t  mphf_chm_imp_build_start (mphf_t *mphf);
ssize_t  mphf_chm_imp_build_end   (mphf_t *mphf);
ssize_t  mphf_chm_imp_clean       (mphf_t *mphf);
ssize_t  mphf_chm_imp_destroy     (mphf_t *mphf);
ssize_t  mphf_chm_imp_insert      (mphf_t *mphf, uint64_t key, uint64_t  value);
ssize_t  mphf_chm_imp_query       (mphf_t *mphf, uint64_t key, uint64_t *value);
ssize_t  mphf_chm_imp_delete      (mphf_t *mphf, uint64_t key);

#endif
