#include <libfrozen.h>
#include <backends/mphf/mphf.h>
#include <backends/mphf/mphf_chm_imp.h>

typedef struct chm_imp_params_t {
	uint32_t            hash1;
	//uint32_t            hash2;
} chm_imp_params_t;

typedef struct chm_imp_gparams_t { // TODO move to params_t
	uint64_t            last_edge;
} chm_imp_gparams_t;

typedef struct chm_imp_build_data_t {
	backend_t          *backend;
	uint64_t            gp_offset;
	uint64_t            gv_offset;
	uint64_t            ge_offset;
} chm_imp_build_data_t;

typedef struct chm_imp_t {
	uint32_t            filled;
	
	chm_imp_params_t    params;
	mphf_hash_proto_t  *hash;
	uint64_t            nvertex;
	uint32_t            bt_value;
	uint32_t            bt_vertex;
	uint32_t            bi_vertex;
	uint32_t            persistent;
	
	uint64_t            store_size;
	uint64_t            store_gp_offset; // graph params
	uint64_t            store_gv_offset; // graph vertexes
	uint64_t            store_ge_offset; // graph edges
} chm_imp_t;

typedef struct graph_egde_t {
	uint64_t  vertex [2];
	uint64_t  next   [2];
} graph_egde_t;

typedef struct vertex_list_t {
	uint64_t               vertex;
	struct vertex_list_t  *next;
} vertex_list_t;

#define CHM_CONST 209
#define N_INITIAL_DEFAULT    256
#define VALUE_BITS_DEFAULT   32
#define HASH_TYPE_DEFAULT    "jenkins"
#define BITS_TO_BYTES(x) ( ((x - 1) >> 3) + 1)

static char clean[100] = {0};

// TIP nice memory saving is: store edges in vertex table with additional bit field, if
//     it set - edge exist and vertex field contain edge_id, if not - vertex field contain
//     second vertex for stored edge. However this method have complex background - adding edge
//     invalid in commented code

static uint64_t log_any(uint64_t x, uint64_t power){
	uint64_t res   = 0;
	uint64_t t     = power;
	
	while (t <= x){
		++res;
		t *= power;
	}
	return res + 1;
}

static ssize_t fill_data       (mphf_t *mphf){ // {{{
	ssize_t    ret;
	uint64_t   nvertex;
	DT_INT32   persistent    = 0;
	DT_INT64   nelements     = N_INITIAL_DEFAULT;
	DT_INT32   bvalue        = VALUE_BITS_DEFAULT;
	DT_STRING  hash_type_str = HASH_TYPE_DEFAULT;
	chm_imp_t *data          = (chm_imp_t *)&mphf->data;
	
	if((data->filled & 0x1) != 0)
		return 0;
	
	hash_copy_data(ret, TYPE_INT32,  persistent,    mphf->config, HK(persistent)); // if 1 - graph stored in file
	hash_copy_data(ret, TYPE_INT64,  nelements,     mphf->config, HK(nelements));  // number of elements
	hash_copy_data(ret, TYPE_INT32,  bvalue,        mphf->config, HK(value_bits)); // number of bits per value to store
	hash_copy_data(ret, TYPE_STRING, hash_type_str, mphf->config, HK(hash));       // hash function
	
	if((data->hash = mphf_string_to_hash_proto(hash_type_str)) == NULL)
		return -EINVAL;
	
	// nvertex = (2.09 * nelements)
	nvertex = nelements;
	nvertex = (__MAX(uint64_t) / CHM_CONST > nvertex) ? (nvertex * CHM_CONST) / 100 : (nvertex / 100) * CHM_CONST;
	
	data->nvertex     = nvertex;
	data->bi_vertex   = log_any(nvertex, 2);
	data->bt_vertex   = BITS_TO_BYTES(data->bi_vertex);
	data->bt_value    = BITS_TO_BYTES(bvalue);
	data->persistent  = persistent;
	
	if(__MAX(uint64_t) / data->bt_value <= nvertex)
		return -EINVAL;
	
	data->store_size  = sizeof(*data) + nvertex * data->bt_value;
	
	if(persistent != 0){
		// TODO validate 
		data->store_gp_offset = data->store_size;
		data->store_gv_offset = data->store_gp_offset + sizeof(chm_imp_gparams_t);
		data->store_ge_offset = data->store_gv_offset + nvertex   * data->bt_vertex;
		data->store_size      = data->store_ge_offset + nelements * data->bt_vertex * 4; 
	}
	
	data->filled |= 0x1;
	return 0;
} // }}}
static ssize_t fill_params     (mphf_t *mphf){ // {{{
	chm_imp_t *data          = (chm_imp_t *)&mphf->data;
	
	if((data->filled & 0x2) != 0)
		return 0;
	
	data->filled |= 0x2;
	return mphf_store_read(mphf->backend, mphf->offset, &data->params, sizeof(data->params));
} // }}}

static ssize_t graph_new_edge_id(mphf_t *mphf, uint64_t *new_id){ // {{{
	chm_imp_build_data_t  *build_data;
	chm_imp_gparams_t      gparams;
	
	build_data = mphf->build_data;
	
	if(mphf_store_read  (build_data->backend, build_data->gp_offset, &gparams, sizeof(gparams)) < 0)
		return -EFAULT;
	
	*new_id = ++gparams.last_edge; // first edge will be 1, 0-edge reserved
	
	if(mphf_store_write (build_data->backend, build_data->gp_offset, &gparams, sizeof(gparams)) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t graph_get_edge(mphf_t *mphf, uint64_t id, graph_egde_t *edge){ // {{{
	uint64_t               sizeof_edge, vertex_mask;
	char                   buffer[4 * sizeof(uint64_t)];
	chm_imp_build_data_t  *build_data = mphf->build_data;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	sizeof_edge  = 4 * data->bt_vertex;
	vertex_mask  = ( 1 << (data->bt_vertex << 3) ) - 1;
	
	if(mphf_store_read(
		build_data->backend,
		build_data->ge_offset + (id - 1) * sizeof_edge,
		&buffer,
		sizeof_edge
	) < 0)
		return -EFAULT;
	
	edge->vertex[0] = (*(uint64_t *)(buffer                                           )) & vertex_mask;
	edge->vertex[1] = (*(uint64_t *)(buffer +  data->bt_vertex                        )) & vertex_mask;
	edge->next[0]   = (*(uint64_t *)(buffer + (data->bt_vertex << 1)                  )) & vertex_mask;
	edge->next[1]   = (*(uint64_t *)(buffer + (data->bt_vertex << 1) + data->bt_vertex)) & vertex_mask;
	
	return 0;
} // }}}
static ssize_t graph_set_edge(mphf_t *mphf, uint64_t id, graph_egde_t *edge){ // {{{
	uint64_t               edge_size;
	char                   buffer[4 * sizeof(uint64_t)];
	chm_imp_build_data_t  *build_data = mphf->build_data;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	edge_size    = 4 * data->bt_vertex;
	
	*(uint64_t *)(buffer                                           ) = edge->vertex[0];
	*(uint64_t *)(buffer +  data->bt_vertex                        ) = edge->vertex[1];
	*(uint64_t *)(buffer + (data->bt_vertex << 1)                  ) = edge->next[0];
	*(uint64_t *)(buffer + (data->bt_vertex << 1) + data->bt_vertex) = edge->next[1];
	
	if(mphf_store_write(
		build_data->backend,
		build_data->ge_offset + (id - 1) * edge_size,
		&buffer,
		edge_size
	) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t graph_get_first(mphf_t *mphf, size_t nvertex, uint64_t *vertex, uint64_t *edge_id){ // {{{
	size_t                 i;
	chm_imp_build_data_t  *build_data = mphf->build_data;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	for(i=0; i<nvertex; i++){
		edge_id[i] = 0;
		
		if(mphf_store_read(
			build_data->backend,
			build_data->gv_offset + vertex[i] * data->bt_vertex,
			&edge_id[i],
			data->bt_vertex
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_set_first(mphf_t *mphf, size_t nvertex, uint64_t *vertex, uint64_t *edge_id){ // {{{
	size_t                 i;
	chm_imp_build_data_t  *build_data = mphf->build_data;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	for(i=0; i<nvertex; i++){
		if(mphf_store_write(
			build_data->backend,
			build_data->gv_offset + vertex[i] * data->bt_vertex,
			&edge_id[i],
			data->bt_vertex
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_getg(mphf_t *mphf, size_t nvertex, uint64_t *vertex, uint64_t *g){ // {{{
	size_t                 i;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	for(i=0; i<nvertex; i++){
		g[i] = 0;
		
		if(mphf_store_read(
			mphf->backend,
			sizeof(data->params) + vertex[i] * data->bt_value,
			&g[i],
			data->bt_value
		) < 0)
			return -EFAULT;
	}
	return 0;

} // }}}
static ssize_t graph_setg(mphf_t *mphf, size_t nvertex, uint64_t *vertex, uint64_t *new_g){ // {{{
	size_t                 i;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;

	for(i=0; i<nvertex; i++){
		if(mphf_store_write(
			mphf->backend,
			sizeof(data->params) + vertex[i] * data->bt_value,
			&new_g[i],
			data->bt_value
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_recalc(mphf_t *mphf, vertex_list_t *vertex, uint64_t g_old, uint64_t g_new, uint64_t skip_edge){ // {{{
	ssize_t                ret;
	size_t                 our_vertex, frn_vertex;
	uint64_t               edge_id, frn_g_old, frn_g_new;
	graph_egde_t           edge;
	vertex_list_t          vertex_new, *curr;
	
	//printf("recalc called: %llx, g_old: %llx g_new: %llx\n",
	//	vertex,  g_old, g_new
	//);
	
	for(curr = vertex->next; curr; curr = curr->next){
		if(curr->vertex == vertex->vertex){
			return -EBADF;
		}
	}
	
	if(graph_get_first(mphf, 1, &vertex->vertex, &edge_id) < 0)
		return -EFAULT;
	
	for(; edge_id != 0; edge_id = edge.next[our_vertex]){
		if(graph_get_edge(mphf, edge_id, &edge) < 0)
			return -EFAULT;
		
		      if(edge.vertex[0] == vertex->vertex){ our_vertex = 0; frn_vertex = 1;
		}else if(edge.vertex[1] == vertex->vertex){ our_vertex = 1; frn_vertex = 0;
		}else return -EFAULT;
		
		//printf("recalc edge: %llx -> v:{%llx,%llx:%llx,%llx} next: %llx\n",
		//	edge_id, edge.vertex[0], edge.vertex[1], edge.next[0], edge.next[1],
		//	edge.next[our_vertex]
		//);
		
		if(skip_edge == edge_id)
			continue;
		
		if(graph_getg(mphf, 1, &edge.vertex[frn_vertex], &frn_g_old) < 0)
			return -EFAULT;
		
		frn_g_new = g_old + frn_g_old - g_new;
		
		vertex_new.vertex = edge.vertex[frn_vertex];
		vertex_new.next   = vertex;
		
		if((ret = graph_recalc(mphf, &vertex_new, frn_g_old, frn_g_new, edge_id)) < 0)
			return ret;
	}
	
	if(graph_setg(mphf, 1, &vertex->vertex, &g_new) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t graph_add_edge(mphf_t *mphf, uint64_t *vertex, uint64_t value){ // {{{
	ssize_t                ret;
	uint64_t               new_edge_id, g_new, g_free = __MAX(uint64_t);
	uint64_t               tmp[2];
	graph_egde_t           new_edge;
	vertex_list_t          vertex_new;
	
	if(graph_new_edge_id(mphf, &new_edge_id) < 0)
		return -EFAULT;
	
	new_edge.vertex[0] = vertex[0];
	new_edge.vertex[1] = vertex[1];
	
	if(graph_get_first(mphf, 2, vertex, (uint64_t *)&new_edge.next) < 0)
		return -EFAULT;
	
	// save edge
	if(graph_set_edge(mphf, new_edge_id, &new_edge) < 0)
		return -EFAULT;
	
	tmp[0] = tmp[1] = new_edge_id;
	if(graph_set_first(mphf, 2, vertex, (uint64_t *)&tmp) < 0)
		return -EFAULT;
	
	// update g
	      if(new_edge.next[0] == 0){
		if(graph_getg(mphf, 1, &vertex[1], (uint64_t *)&tmp[1]) < 0)
			return -EFAULT;
	      	
		g_free = 0;
		g_new  = value - tmp[1];
	}else if(new_edge.next[1] == 0){
		if(graph_getg(mphf, 1, &vertex[0], (uint64_t *)&tmp[0]) < 0)
			return -EFAULT;
		
		g_free = 1;
		g_new  = value - tmp[0];
	}else{
		if(graph_getg(mphf, 2, vertex, (uint64_t *)&tmp) < 0)
			return -EFAULT;
		
		g_free = 1;
		g_new  = value - tmp[0];
		
		vertex_new.vertex = new_edge.vertex[g_free];
		vertex_new.next   = NULL;
		
		if( (ret = graph_recalc(mphf, &vertex_new, tmp[1], g_new, new_edge_id)) < 0)
			return ret;
		
		return 0;
	}
	
	if(graph_setg(mphf, 1, &vertex[g_free], &g_new) < 0)
		return -EFAULT;
	
	return 0;
} // }}}

ssize_t mphf_chm_imp_new         (mphf_t *mphf){ // {{{
	chm_imp_t *data = (chm_imp_t *)&mphf->data;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(mphf_store_new   (mphf->backend, &mphf->offset, data->store_size) < 0) return -EFAULT;
	
	return mphf_chm_imp_clean(mphf);
} // }}}
ssize_t mphf_chm_imp_build_start (mphf_t *mphf){ // {{{
	chm_imp_build_data_t *build_data;
	chm_imp_t            *data = (chm_imp_t *)&mphf->data;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(data->persistent != 0){
		// store data in file
		mphf->build_data = build_data = malloc(sizeof(chm_imp_build_data_t));
		build_data->backend   = mphf->backend;
		build_data->gp_offset = data->store_gp_offset;
		build_data->gv_offset = data->store_gv_offset;
		build_data->ge_offset = data->store_ge_offset;
		return 0;
	}
	// TODO store data in memory
	return -EINVAL;
} // }}}
ssize_t mphf_chm_imp_build_end   (mphf_t *mphf){ // {{{
	chm_imp_t            *data = (chm_imp_t *)&mphf->data;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(data->persistent != 0){
		free(mphf->build_data);
		mphf->build_data = NULL;
		return 0;
	}
	// TODO release memory
	return -EINVAL;
} // }}}
ssize_t mphf_chm_imp_clean       (mphf_t *mphf){ // {{{
	chm_imp_t            *data = (chm_imp_t *)&mphf->data;
	
	data->filled = 0;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(mphf_store_fill  (mphf->backend, mphf->offset, &clean, sizeof(clean), data->store_size)  < 0) return -EFAULT;
	
	srandom(time(NULL));
	data->params.hash1 = random();
	//data->params.hash2 = random();
	
	if(mphf_store_write (mphf->backend, mphf->offset, &data->params, sizeof(data->params)) < 0) return -EFAULT;
	//if(mphf_store_write(mphf->backend, mphf->offset + data.store_gp_offset, &data...., sizeof(chm_imp_gparams_t)) < 0) return -EFAULT;
	
	if(fill_params(mphf) < 0)
		return -EINVAL;
	
	return 0;
} // }}}
ssize_t mphf_chm_imp_destroy     (mphf_t *mphf){ // {{{
	chm_imp_t            *data = (chm_imp_t *)&mphf->data;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(mphf_store_delete(mphf->backend, mphf->offset, data->store_size) < 0)
		return -EFAULT;
	
	return 0;
} // }}}

ssize_t mphf_chm_imp_insert (mphf_t *mphf, char *key, size_t key_len, uint64_t  value){ // {{{
	uint32_t               hash[2];
	uint64_t               vertex[2];
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(fill_params(mphf) < 0)
		return -EINVAL;
	
	// TODO 64-bit
	data->hash->func_hash32(data->params.hash1, key, key_len, (uint32_t *)&hash, 2);
	//data->hash->func_hash32(data->params.hash2, key, key_len, (uint32_t *)&hash[1], 1);
	
	vertex[0] = (hash[0] % data->nvertex);
	vertex[1] = (hash[1] % data->nvertex);
	//vertex[1] = ((hash[1] ^ data->params.hash2) % data->nvertex);
	
	if(vertex[0] == vertex[1])
		return -EBADF;
	
	//printf("mphf: insert: %.*s value: %.8llx v:{%llx,%llx:%x:%d}\n",
	//	key_len, key, value, vertex[0], vertex[1], data->params.hash1, key_len
	//);
	
	return graph_add_edge(mphf, (uint64_t *)&vertex, value);
} // }}}
ssize_t mphf_chm_imp_query  (mphf_t *mphf, char *key, size_t key_len, uint64_t *value){ // {{{
	uint32_t               hash[2];
	uint64_t               vertex[2], g[2];
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	if(fill_data(mphf) < 0)
		return -EINVAL;
	
	if(fill_params(mphf) < 0)
		return -EINVAL;
	
	// TODO 64-bit
	data->hash->func_hash32(data->params.hash1, key, key_len, (uint32_t *)&hash, 2);
	//data->hash->func_hash32(data->params.hash2, key, key_len, (uint32_t *)&hash[1], 1);
	
	vertex[0] = (hash[0] % data->nvertex);
	vertex[1] = (hash[1] % data->nvertex);
	//vertex[1] = ((hash[1] ^ data->params.hash2) % data->nvertex);
	
	if(vertex[0] == vertex[1])
		return MPHF_QUERY_NOTFOUND;
	
	if(graph_getg(mphf, 2, (uint64_t *)&vertex, (uint64_t *)&g) < 0)
		return MPHF_QUERY_NOTFOUND;
	
	*value = (g[0] + g[1]) & ( ( 1 << (data->bt_vertex << 3) ) - 1);
	
	//printf("mphf: query %.*s value: %.8llx v:{%llx,%llx:%x:%d}\n",
	//	key_len, key, *value, vertex[0], vertex[1], data.params.hash1, key_len
	//);
	return MPHF_QUERY_FOUND;
} // }}}
ssize_t mphf_chm_imp_delete (mphf_t *mphf, char *key, size_t key_len){ // {{{
	return -EINVAL;
} // }}}

/* Invalid code {{{
typedef struct graph_egde_t {
	// public
	uint64_t  second_vertex;
	uint64_t  own_vertex_list;
	uint64_t  frn_vertex_list;
	// private
	uint64_t  edge_id;
} graph_egde_t;

// gv list: [1 bit; {bit=0 -> second vertex; bit=1 -> edge_id }]
// ge list: [second vertex; own vertex's edge list; foreign vertex's edge list]

static ssize_t graph_new_edge_id(mphf_t *mphf, uint64_t *new_id){ // {{{
	chm_imp_build_data_t  *build_data;
	chm_imp_gparams_t      gparams;
	
	build_data = mphf->build_data;
	
	if(mphf_store_read  (build_data->backend, build_data->gp_offset, &gparams, sizeof(gparams)) < 0)
		return -EFAULT;
	
	*new_id = gparams.last_edge++;
	
	if(mphf_store_write (build_data->backend, build_data->gp_offset, &gparams, sizeof(gparams)) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t graph_get_edge(mphf_t *mphf, chm_imp_t *data, size_t nid, uint64_t *id, graph_egde_t *edge){ // {{{
	uint64_t               i, t, k, sizeof_edge, vertex_mask, edgebit_mask;
	char                   buffer[3 * sizeof(uint64_t)];
	chm_imp_build_data_t  *build_data = mphf->build_data;
	
	sizeof_edge  = 3 * data->bt_vertex;
	vertex_mask  = ( 1 << (data->bt_vertex << 3) ) - 1;
	edgebit_mask = ( 1 << data->bi_vertex );
	
	for(i=0; i < nid; i++){
		// read from vertex array
		if(mphf_store_read(
			build_data->backend,
			build_data->gv_offset + id[i] * data->bt_gv,
			&buffer,
			data->bt_gv
		) < 0)
			return -EFAULT;
		
		t = *(uint64_t *)(buffer);
		k = (t & (edgebit_mask - 1));
		
		if( (t & edgebit_mask) == 0){
			edge[i].second_vertex   = k;
			edge[i].edge_id         = __MAX(uint64_t);
			edge[i].own_vertex_list = 0;
			edge[i].frn_vertex_list = 0;
		}else{
			edge[i].edge_id         = k;
			
			if(mphf_store_read(
				build_data->backend,
				build_data->ge_offset + k * sizeof_edge,
				&buffer,
				sizeof_edge
			) < 0)
				return -EFAULT;
			
			edge[i].second_vertex   = (*(uint64_t *)(buffer                         )) & vertex_mask;
			edge[i].own_vertex_list = (*(uint64_t *)(buffer +  data->bt_vertex      )) & vertex_mask;
			edge[i].frn_vertex_list = (*(uint64_t *)(buffer + (data->bt_vertex << 1))) & vertex_mask;
		}
	}
	return 0;
} // }}}
static ssize_t graph_set_edge(mphf_t *mphf, chm_imp_t *data, size_t nid, uint64_t *id, graph_egde_t *edge){ // {{{
	uint64_t               i, edge_size;
	char                   buffer[3 * sizeof(uint64_t)];
	chm_imp_build_data_t  *build_data = mphf->build_data;
	
	edge_size    = 3 * data->bt_vertex;
	
	for(i=0; i < nid; i++){
		if(edge[i].own_vertex_list != 0 || edge[i].frn_vertex_list != 0){

			// create new edge if not exist
			if(edge[i].edge_id == __MAX(uint64_t)){
				if(graph_new_edge_id(mphf, &edge[i].edge_id) < 0)
					return -EFAULT;
			}
			
			*(uint64_t *)(buffer                         ) = edge[i].second_vertex;
			*(uint64_t *)(buffer +  data->bt_vertex      ) = edge[i].own_vertex_list;
			*(uint64_t *)(buffer + (data->bt_vertex << 1)) = edge[i].frn_vertex_list;
			
			if(mphf_store_write(
				build_data->backend,
				build_data->ge_offset + edge[i].edge_id * edge_size,
				&buffer,
				edge_size
			) < 0)
				return -EFAULT;
			
			*(uint64_t *)(buffer)  = edge[i].edge_id;
			*(uint64_t *)(buffer) |= ( 1 << data->bi_vertex );
		}else{
			*(uint64_t *)(buffer)  = edge[i].second_vertex;
		}
		
		// write to vertex array
		if(mphf_store_write(
			build_data->backend,
			build_data->gv_offset + id[i] * data->bt_gv,
			&buffer,
			data->bt_gv
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_getg(mphf_t *mphf, chm_imp_t *data, size_t nvertex, uint64_t *vertex, uint64_t *g){ // {{{
	size_t i;
	
	for(i=0; i<nvertex; i++){
		g[i] = 0;
		
		if(mphf_store_read(
			mphf->backend,
			sizeof(data->params) + vertex[i] * data->bt_value,
			&g[i],
			data->bt_value
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_setg(mphf_t *mphf, chm_imp_t *data, size_t nvertex, uint64_t *vertex, uint64_t *new_g){ // {{{
	size_t i;

	for(i=0; i<nvertex; i++){
		if(mphf_store_write(
			mphf->backend,
			sizeof(data->params) + vertex[i] * data->bt_value,
			&new_g[i],
			data->bt_value
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_recalc(mphf_t *mphf, chm_imp_t *data, uint64_t initial_vertex, uint64_t from_vertex, graph_egde_t *parent, uint64_t g_old, uint64_t g_new){ // {{{
	uint64_t       child_vertex, child_g_old, child_g_new;
	graph_egde_t   child;

	child_vertex = parent->own_vertex_list;
	printf("parent: %llx -> { %llx, %llx, %llx }\n", from_vertex, parent->second_vertex, parent->own_vertex_list, parent->frn_vertex_list);
	while(child_vertex != 0){
		if(child_vertex == from_vertex)
			return 0;
		printf("%llx <> %llx\n", child_vertex, from_vertex);

		//if(child_vertex == initial_vertex)
		//	return -EBADF;
		
		if(graph_get_edge(mphf, data, 1, &child_vertex, &child) < 0)
			return -EFAULT;
		
		printf("child:  %llx -> { %llx, %llx, %llx }\n", child_vertex, child.second_vertex, child.own_vertex_list, child.frn_vertex_list);
		
		if(graph_getg(mphf, data, 1, &child.second_vertex, &child_g_old) < 0)
			return -EFAULT;
		
		child_g_new = g_old + child_g_old - g_new;
		
		if(graph_recalc(mphf, data, initial_vertex, child.own_vertex_list, (graph_egde_t *)&child, child_g_old, child_g_new) < 0)
			return -EFAULT;
		
		if(graph_setg(mphf, data, 1, &child.second_vertex, &child_g_new) < 0)
			return -EFAULT;
		
		child_vertex = child.frn_vertex_list;
		printf("next\n");
	}
	return 0;
} // }}}
static ssize_t graph_add_edge(mphf_t *mphf, chm_imp_t *data, uint64_t *vertex, uint64_t value){ // {{{
	ssize_t        ret;
	graph_egde_t   edges[2];
	
	if(graph_get_edge(mphf, data, 2, vertex, (graph_egde_t *)&edges) < 0){
		printf("get_edge failed\n");
		return -EFAULT;
	}
	
	edges[0].frn_vertex_list = edges[1].own_vertex_list = edges[1].second_vertex;
	edges[1].frn_vertex_list = edges[0].own_vertex_list = edges[0].second_vertex;
	edges[0].second_vertex   = vertex[1];
	edges[1].second_vertex   = vertex[0];
	
	if(graph_set_edge(mphf, data, 2, vertex, (graph_egde_t *)&edges) < 0){
		printf("set_edge failed\n");
		return -EFAULT;
	}
	
	uint64_t g[2], g_new, g_free = __MAX(uint64_t);
	
	if(edges[0].own_vertex_list == 0){
		g_free = 0;
		g[0]   = 0;
	}else{
		if(graph_getg(mphf, data, 1, &vertex[0], &g[0]) < 0)
			return -EFAULT;
	}
	
	if(edges[1].own_vertex_list == 0){
		g_free = 1;
		g[1]   = 0;
	}else{
		if(graph_getg(mphf, data, 1, &vertex[1], &g[1]) < 0)
			return -EFAULT;
	}
	
	if(g_free == __MAX(uint64_t)){
		g_free = 1;
		g_new  = value - g[0]; // g_new
		
		if( (ret = graph_recalc(mphf, data, vertex[1], vertex[1], &edges[1], g[1], g_new)) < 0)
			return ret;
	}else{
		g_new = value - (g[0] + g[1]);
	}
	
	if(graph_setg(mphf, data, 1, &vertex[g_free], &g_new) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
}}} */
