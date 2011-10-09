#include <libfrozen.h>
#include <mphf.h>
#include <mphf_chm_imp.h>

typedef enum chm_imp_fill_flags {
	FILLED                 = 1,
	WRITEABLE              = 2
} chm_imp_fill_flags;

typedef struct chm_imp_params_t {
	uint64_t               capacity;  // current mphf capacity
	uint64_t               nelements; // real items count. can be less than real
	uint64_t               hash1;
	uint64_t               hash2;
} chm_imp_params_t;

typedef struct chm_imp_t {
	uintmax_t              status;
	
	backend_t             *be_g;
	backend_t             *be_v;
	backend_t             *be_e;
	
	chm_imp_params_t       params;
	uintmax_t              nelements_min;  // minimum required capacity
	uintmax_t              nelements_step; // minimum capacity step
	uintmax_t              nelements_mul; // capacity step multiplexer
	uintmax_t              nvertex;
	uintmax_t              bi_value;
	uintmax_t              bt_value;
	uintmax_t              bt_vertex;
	uintmax_t              bi_vertex;
} chm_imp_t;

typedef struct graph_edge_t {
	uintmax_t              vertex [2];
	uintmax_t              next   [2];
} graph_edge_t;

typedef struct vertex_list_t {
	uintmax_t              vertex;
	struct vertex_list_t  *next;
} vertex_list_t;

#define EMODULE               11
#define CHM_CONST             209
#define REBUILD_CONST         80     // in case of rebuild if array size is <REBUILD_CONST>% of capacity of array - it expands
#define CAPACITY_MIN_DEFAULT  256
#define CAPACITY_STEP_DEFAULT 256
#define CAPACITY_MUL_DEFAULT  1
#define KEY_BITS_DEFAULT      32
#define VALUE_BITS_DEFAULT    32
#define BITS_TO_BYTES(x) ( ((x - 1) >> 3) + 1)

//#define MPHF_DEBUG

static char clean[100] = {0};

// TIP nice memory saving is: store edges in vertex table with additional bit field, if
//     it set - edge exist and vertex field contain edge_id, if not - vertex field contain
//     second vertex for stored edge. However this method have complex background - adding edge
//     invalid in commented code

static uintmax_t log_any(uintmax_t x, uintmax_t power){ // {{{
	uintmax_t res   = 0;
	uintmax_t t     = power;
	
	while (t <= x){
		++res;
		t *= power;
	}
	return res + 1;
} // }}}

static ssize_t chm_imp_param_read (mphf_t *mphf){ // {{{
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	fastcall_read r_read = { { 5, ACTION_READ }, 0, &data->params, sizeof(data->params) };
	if(backend_fast_query(data->be_g, &r_read) < 0){
		memset(&data->params, 0, sizeof(data->params));
		return -EFAULT;
	}
	
	return 0;
} // }}}
static ssize_t chm_imp_param_write(mphf_t *mphf){ // {{{
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &data->params, sizeof(data->params) };
	if(backend_fast_query(data->be_g, &r_write) < 0)
		return error("params write failed");
	
	return 0;
} // }}}
static ssize_t chm_imp_file_wipe  (mphf_t *mphf){ // {{{
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	fastcall_count r_count = { { 3, ACTION_COUNT } };
	
	// kill g
	r_count.nelements = 0;
	backend_fast_query(data->be_g, &r_count);
	if(r_count.nelements != 0){
		fastcall_delete r_delete = { { 4, ACTION_DELETE }, 0, r_count.nelements };
		if(backend_fast_query(data->be_g, &r_delete) < 0)
			return error("g array delete failed");
	}
	
	// kill e
	r_count.nelements = 0;
	backend_fast_query(data->be_e, &r_count);
	if(r_count.nelements != 0){
		fastcall_delete r_delete = { { 4, ACTION_DELETE }, 0, r_count.nelements };
		if(backend_fast_query(data->be_e, &r_delete) < 0)
			return error("g array delete failed");
	}
	
	// kill v
	r_count.nelements = 0;
	backend_fast_query(data->be_v, &r_count);
	if(r_count.nelements != 0){
		fastcall_delete r_delete = { { 4, ACTION_DELETE }, 0, r_count.nelements };
		if(backend_fast_query(data->be_v, &r_delete) < 0)
			return error("g array delete failed");
	}
	return 0;
} // }}}
static ssize_t chm_imp_file_one_init(backend_t *backend, uintmax_t size){ // {{{
	uintmax_t              fill_size;
	
	fastcall_create r_create = { { 4, ACTION_CREATE }, size };
	if(backend_fast_query(backend, &r_create) < 0)
		return -1;
	
	if(r_create.offset != 0)                                 // TODO remove this
		return -1;
	
	do{
		fill_size = MIN(size, sizeof(clean));
		
		fastcall_write r_write = { { 5, ACTION_WRITE }, r_create.offset, &clean, fill_size };
		if(backend_fast_query(backend, &r_write) != 0)
			return -1;
		
		r_create.offset  += r_write.buffer_size;
		size             -= r_write.buffer_size;
	}while(size > 0);
	return 0;
} // }}}
static ssize_t chm_imp_file_init  (mphf_t *mphf){ // {{{
	ssize_t                ret;
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	if( (ret = chm_imp_file_wipe(mphf)) < 0)
		return ret;
	
	// fill g
	if(chm_imp_file_one_init(data->be_g, sizeof(data->params) + data->nvertex * data->bt_value) != 0)
		return error("g array init failed");
	
	// fill v
	if(chm_imp_file_one_init(data->be_v, data->nvertex * data->bt_vertex) != 0)
		return error("v array init failed");
	
	// fill e
	// empty
	return 0;
} // }}}
static ssize_t chm_imp_configure  (mphf_t *mphf, request_t *fork_req){ // {{{
	ssize_t                ret;
	backend_t             *t;
	backend_t             *be_g              = NULL;
	backend_t             *be_v              = NULL;
	backend_t             *be_e              = NULL;
	char                  *backend           = NULL;
	uintmax_t              nelements_min     = CAPACITY_MIN_DEFAULT;
	uintmax_t              nelements_step    = CAPACITY_STEP_DEFAULT;
	uintmax_t              nelements_mul     = CAPACITY_MUL_DEFAULT;
	uintmax_t              bi_value          = VALUE_BITS_DEFAULT;
	uintmax_t              readonly          = 0;
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	if( (data->status & FILLED) == 0){
		hash_data_copy(ret, TYPE_UINTT,  nelements_min,  mphf->config, HK(nelements_min));
		hash_data_copy(ret, TYPE_UINTT,  nelements_step, mphf->config, HK(nelements_step));
		hash_data_copy(ret, TYPE_UINTT,  nelements_mul,  mphf->config, HK(nelements_mul));
		hash_data_copy(ret, TYPE_UINTT,  bi_value,       mphf->config, HK(value_bits));     // number of bits per value to store
		hash_data_copy(ret, TYPE_UINTT,  readonly,       mphf->config, HK(readonly));       // run in read-only mode
		
		hash_data_copy(ret, TYPE_STRINGT, backend,       mphf->config, HK(backend_g));
		if(ret == 0){
			be_g = t = backend_acquire(backend);
			if(fork_req){
				be_g = backend_fork(t, fork_req);
				backend_destroy(t);
			}
		}
		
		hash_data_copy(ret, TYPE_STRINGT, backend,       mphf->config, HK(backend_v));
		if(ret == 0){
			be_v = t = backend_acquire(backend);
			if(fork_req){
				be_v = backend_fork(t, fork_req);
				backend_destroy(t);
			}
		}
		
		hash_data_copy(ret, TYPE_STRINGT, backend,       mphf->config, HK(backend_e));
		if(ret == 0){
			be_e = t = backend_acquire(backend);
			if(fork_req){
				be_e = backend_fork(t, fork_req);
				backend_destroy(t);
			}
		}
		
		
		if(be_g == NULL)
			return error("backend chm_imp parameter backend_g invalid");
		
		if(be_v == NULL || be_e == NULL || readonly != 0){
			data->status &= ~WRITEABLE;
		}else{
			data->status |= WRITEABLE;
		}
		
		data->be_g           = be_g;
		data->be_v           = be_v;
		data->be_e           = be_e;
		data->nelements_min  = nelements_min;
		data->nelements_step = nelements_step;
		data->nelements_mul  = nelements_mul;
		data->bi_value       = bi_value;
		data->bt_value       = BITS_TO_BYTES(data->bi_value);
		
		data->status        |= FILLED;
	}
	return 0;
} // }}}
static ssize_t chm_imp_configure_r(mphf_t *mphf, uint64_t capacity){ // {{{
	uintmax_t              nvertex;
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	if(capacity == 0) capacity = 1; // at least one element
	
	// nvertex = (2.09 * capacity)
	if(safe_mul(&nvertex, capacity, CHM_CONST) < 0){
		safe_div(&nvertex, capacity, 100);
		
		if(safe_mul(&nvertex, nvertex, CHM_CONST) < 0)
			return error("too many elements");
	}
	safe_div(&nvertex, nvertex, 100);
	
	data->nvertex            = nvertex;
	data->params.capacity    = capacity;
	data->bi_vertex          = log_any(nvertex, 2);
	data->bt_vertex          = BITS_TO_BYTES(data->bi_vertex);
	return 0;
} // }}}
static ssize_t chm_imp_load       (mphf_t *mphf, request_t *fork_req){ // {{{
	ssize_t                ret;
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	// configure initial parameters for mphf
	if( (ret = chm_imp_configure(mphf, fork_req)) < 0)
		return ret;
	
	if( (ret = chm_imp_param_read(mphf)) < 0)
		goto rebuild; // no params on disk found - rebuild array
	
	if( (ret = chm_imp_configure_r(mphf, data->params.capacity)) < 0)
		return ret;
	
	// check if array capacity match new _min requirements
	if(data->params.capacity < data->nelements_min)
		goto rebuild;
	
	return 0;

rebuild:
	// if mphf not exist - run rebuild with minimum nelements
	return mphf_chm_imp_rebuild(mphf);
} // }}}
static ssize_t chm_imp_check      (mphf_t *mphf, size_t flags){ // {{{
	chm_imp_t             *data              = (chm_imp_t *)&mphf->data;
	
	if( (flags & WRITEABLE) && ((data->status & WRITEABLE) == 0) )
		return error("mphf is read-only");
	
	return 0;
} // }}}
static ssize_t chm_imp_unload     (mphf_t *mphf){ // {{{
	chm_imp_t             *data  = (chm_imp_t *)&mphf->data;
	
	// save .params
	chm_imp_param_write(mphf);
	
	if(data->be_g)
		backend_destroy(data->be_g);
	
	if( (data->status & WRITEABLE) ){
		backend_destroy(data->be_e);
		backend_destroy(data->be_v);
	}
	return 0;
} // }}}

static ssize_t graph_new_edge_id(chm_imp_t *data, uintmax_t *id){ // {{{
	uintmax_t              sizeof_edge;
	
	sizeof_edge = 4 * data->bt_vertex;
	
	fastcall_create r_create = { { 4, ACTION_CREATE }, sizeof_edge };
	if(backend_fast_query(data->be_e, &r_create) < 0)
		return error("new_edge_id failed");
	
	*id = (r_create.offset / sizeof_edge) + 1;
	return 0;
} // }}}
static ssize_t graph_get_first(chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *edge_id){ // {{{
	size_t                 i;
	
	for(i=0; i<nvertex; i++){
		edge_id[i] = 0;
		
		fastcall_read r_read = {
			{ 5, ACTION_READ },
			vertex[i] * data->bt_vertex,
			&edge_id[i],
			data->bt_vertex
		};
		if(backend_fast_query(data->be_v, &r_read) < 0)
			return error("get_first failed");
	}
	return 0;
} // }}}
static ssize_t graph_set_first(chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *edge_id){ // {{{
	size_t                 i;
	
	for(i=0; i<nvertex; i++){
		fastcall_write r_write = {
			{ 5, ACTION_WRITE },
			vertex[i] * data->bt_vertex,
			&edge_id[i],
			data->bt_vertex
		};
		if(backend_fast_query(data->be_v, &r_write)< 0)
			return error("set first failed");
	}
	return 0;
} // }}}
static ssize_t graph_get_edge(chm_imp_t *data, uintmax_t id, graph_edge_t *edge){ // {{{
	uintmax_t              sizeof_edge, vertex_mask;
	char                   buffer[4 * sizeof(uintmax_t)];
	
	sizeof_edge  = 4 * data->bt_vertex;
	vertex_mask  = ( (uintmax_t)1 << (data->bt_vertex << 3) ) - 1;
	
	fastcall_read r_read = {
		{ 5, ACTION_READ },
		(id - 1) * sizeof_edge,
		&buffer,
		sizeof_edge
	};
	if(backend_fast_query(data->be_e, &r_read) < 0)
		return -ENOENT; //error("get_edge failed");
	
	edge->vertex[0] = (*(uintmax_t *)(buffer                                           )) & vertex_mask;
	edge->vertex[1] = (*(uintmax_t *)(buffer +  data->bt_vertex                        )) & vertex_mask;
	edge->next[0]   = (*(uintmax_t *)(buffer + (data->bt_vertex << 1)                  )) & vertex_mask;
	edge->next[1]   = (*(uintmax_t *)(buffer + (data->bt_vertex << 1) + data->bt_vertex)) & vertex_mask;
	
	return 0;
} // }}}
static ssize_t graph_set_edge(chm_imp_t *data, uintmax_t id, graph_edge_t *edge){ // {{{
	uintmax_t              edge_size;
	char                   buffer[4 * sizeof(uintmax_t)];
	
	edge_size    = 4 * data->bt_vertex;
	
	*(uintmax_t *)(buffer                                           ) = edge->vertex[0];
	*(uintmax_t *)(buffer +  data->bt_vertex                        ) = edge->vertex[1];
	*(uintmax_t *)(buffer + (data->bt_vertex << 1)                  ) = edge->next[0];
	*(uintmax_t *)(buffer + (data->bt_vertex << 1) + data->bt_vertex) = edge->next[1];
	
	fastcall_write r_write = {
		{ 5, ACTION_WRITE },
		(id - 1) * edge_size,
		&buffer,
		edge_size
	};
	if(backend_fast_query(data->be_e, &r_write) < 0)
		return error("set_edge failed");
	
	return 0;
} // }}}
static ssize_t graph_find_edge(chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *edge_id){ // {{{
	intmax_t      ret;
	uintmax_t     curr_edge_id;
	graph_edge_t  curr_edge;
	
	if(nvertex != 2)
		return -EINVAL;
	
	curr_edge_id = 0;
	if( (ret = graph_get_first(data, 1, (uintmax_t *)&vertex[0], &curr_edge_id)) < 0)
		return ret;
	
	do{
		if( (ret = graph_get_edge(data, curr_edge_id, &curr_edge)) < 0)
			return ret;
		
		if(memcmp(vertex, curr_edge.vertex, MIN(nvertex * sizeof(vertex[0]), sizeof(curr_edge.vertex))) == 0){
			*edge_id = curr_edge_id;
			return 0;
		}
		
		      if(curr_edge.vertex[0] == vertex[0]){ curr_edge_id = curr_edge.next[0];
		}else if(curr_edge.vertex[1] == vertex[0]){ curr_edge_id = curr_edge.next[1];
		}else return error("find_edge db inconsistency");
	}while(curr_edge_id != 0);
	
	return -ENOENT;
} // }}}
static ssize_t graph_getg(chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *g){ // {{{
	size_t                 i;
	
	for(i=0; i<nvertex; i++){
		g[i] = 0;
		
		fastcall_read r_read = {
			{ 5, ACTION_READ },
			sizeof(data->params) + vertex[i] * data->bt_value,
			&g[i],
			data->bt_value
		};
		if(backend_fast_query(data->be_g, &r_read) < 0)
			return error("get_g failed");
	}
	return 0;

} // }}}
static ssize_t graph_setg(chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *new_g){ // {{{
	size_t                 i;
	
	for(i=0; i<nvertex; i++){
		fastcall_write r_write = {
			{ 5, ACTION_WRITE },
			sizeof(data->params) + vertex[i] * data->bt_value,
			&new_g[i],
			data->bt_value
		};
		if(backend_fast_query(data->be_g, &r_write) < 0)
			return error("set_g failed");
	}
	return 0;
} // }}}
static ssize_t graph_recalc(chm_imp_t *data, vertex_list_t *vertex, uintmax_t g_old, uintmax_t g_new, uintmax_t skip_edge){ // {{{
	ssize_t                ret;
	size_t                 our_vertex, frn_vertex;
	uintmax_t              edge_id, frn_g_old, frn_g_new;
	graph_edge_t           edge;
	vertex_list_t          vertex_new, *curr;
	
	//printf("recalc called: %llx, g_old: %llx g_new: %llx\n",
	//	vertex,  g_old, g_new
	//);
	
	for(curr = vertex->next; curr; curr = curr->next){
		if(curr->vertex == vertex->vertex){
			return -EBADF;
		}
	}
	
	if( (ret = graph_get_first(data, 1, &vertex->vertex, &edge_id)) < 0)
		return ret;
	
	for(; edge_id != 0; edge_id = edge.next[our_vertex]){
		if( (ret = graph_get_edge(data, edge_id, &edge)) < 0)
			return ret;
		
		      if(edge.vertex[0] == vertex->vertex){ our_vertex = 0; frn_vertex = 1;
		}else if(edge.vertex[1] == vertex->vertex){ our_vertex = 1; frn_vertex = 0;
		}else return error("recalc db inconsistency");
		
		//printf("recalc edge: %llx -> v:{%llx,%llx:%llx,%llx} next: %llx\n",
		//	edge_id, edge.vertex[0], edge.vertex[1], edge.next[0], edge.next[1],
		//	edge.next[our_vertex]
		//);
		
		if(skip_edge == edge_id)
			continue;
		
		if( (ret = graph_getg(data, 1, &edge.vertex[frn_vertex], &frn_g_old)) < 0)
			return ret;
		
		frn_g_new = g_old + frn_g_old - g_new;
		
		vertex_new.vertex = edge.vertex[frn_vertex];
		vertex_new.next   = vertex;
		
		if((ret = graph_recalc(data, &vertex_new, frn_g_old, frn_g_new, edge_id)) < 0)
			return ret;
	}
	
	if(( ret = graph_setg(data, 1, &vertex->vertex, &g_new)) < 0)
		return ret;
	
	return 0;
} // }}}
static ssize_t graph_add_edge(chm_imp_t *data, uintmax_t *vertex, uintmax_t value){ // {{{
	ssize_t                ret;
	uintmax_t              new_edge_id, g_new, g_free = __MAX(uintmax_t);
	uintmax_t              tmp[2];
	graph_edge_t           new_edge;
	vertex_list_t          vertex_new;
	
	if( (ret = graph_new_edge_id(data, &new_edge_id)) < 0)
		return ret;
	
	new_edge.vertex[0] = vertex[0];
	new_edge.vertex[1] = vertex[1];
	
	if( (ret = graph_get_first(data, 2, vertex, (uintmax_t *)&new_edge.next)) < 0)
		return ret;
	
	// save edge
	if( (ret = graph_set_edge(data, new_edge_id, &new_edge)) < 0)
		return ret;
	
	tmp[0] = tmp[1] = new_edge_id;
	if( (ret = graph_set_first(data, 2, vertex, (uintmax_t *)&tmp)) < 0)
		return ret;
	
	// update g
	      if(new_edge.next[0] == 0){
		if( (ret = graph_getg(data, 1, &vertex[1], (uintmax_t *)&tmp[1])) < 0)
			return ret;
	      	
		g_free = 0;
		g_new  = value - tmp[1];
	}else if(new_edge.next[1] == 0){
		if( (ret = graph_getg(data, 1, &vertex[0], (uintmax_t *)&tmp[0])) < 0)
			return ret;
		
		g_free = 1;
		g_new  = value - tmp[0];
	}else{
		if( (ret = graph_getg(data, 2, vertex, (uintmax_t *)&tmp)) < 0)
			return ret;
		
		g_free = 1;
		g_new  = value - tmp[0];
		
		vertex_new.vertex = new_edge.vertex[g_free];
		vertex_new.next   = NULL;
		
		if( (ret = graph_recalc(data, &vertex_new, tmp[1], g_new, new_edge_id)) < 0)
			return ret;
		
		goto insert_ok;
	}
	
	if( (ret = graph_setg(data, 1, &vertex[g_free], &g_new)) < 0)
		return ret;

insert_ok:
	data->params.nelements++; // not thread-safe, but still realistic
	
	return 0;
} // }}}

ssize_t mphf_chm_imp_load        (mphf_t *mphf){ // {{{
	return chm_imp_load(mphf, NULL);
} // }}}
ssize_t mphf_chm_imp_unload      (mphf_t *mphf){ // {{{
	return chm_imp_unload(mphf);
} // }}}
ssize_t mphf_chm_imp_fork        (mphf_t *mphf, request_t *request){ // {{{
	return chm_imp_load(mphf, request);
} // }}}
ssize_t mphf_chm_imp_rebuild     (mphf_t *mphf){ // {{{
	ssize_t                ret;
	uint64_t               nelements;
	uint64_t               capacity_curr;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	//                  |  empty array        |  existing array
	// -----------------+---------------------+--------------------------
	// params.nelements |  0                  |  calculated array size
	// params.capacity  |  0                  |  array defined capacity
	// -----------------+---------------------+--------------------------
	
	capacity_curr = data->params.capacity;
	if(
		safe_div(&capacity_curr, capacity_curr, 100) < 0 ||
		safe_mul(&capacity_curr, capacity_curr, REBUILD_CONST) < 0
	)
		capacity_curr = data->params.capacity;
	
	nelements = data->params.nelements;
	
	if(nelements >= capacity_curr){
		// expand array
		nelements  = MAX(MAX(data->params.capacity, data->nelements_min), nelements);
		nelements += data->nelements_step;
		nelements *= data->nelements_mul;
#ifdef MPHF_DEBUG
	printf("mphf expand rebuild on %x (%d) elements\n", (int)nelements, (int)nelements);
#endif
	}else{
		// ordinary rebuild
		nelements = data->params.capacity;
#ifdef MPHF_DEBUG
	printf("mphf normal rebuild on %x (%d) elements\n", (int)nelements, (int)nelements);
#endif
	}
	
	// set .params
	data->params.hash1     = random();
	data->params.hash2     = random();
	data->params.nelements = 0;
	if( (ret = chm_imp_configure_r(mphf, nelements)) < 0)
		return ret;
	
	// reinit files
	if( (ret = chm_imp_file_init(mphf)) < 0)
		return ret;
	
	// save new .params
	if( (ret = chm_imp_param_write(mphf)) < 0)
		return ret;
	
	return 0;
} // }}}
ssize_t mphf_chm_imp_destroy     (mphf_t *mphf){ // {{{
	return chm_imp_file_wipe(mphf);
} // }}}

ssize_t mphf_chm_imp_insert (mphf_t *mphf, uintmax_t key, uintmax_t value){ // {{{
	uintmax_t              vertex[2];
	intmax_t               ret;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	if( (ret = chm_imp_check(mphf, WRITEABLE)) < 0)
		return ret;
	
	vertex[0] = ((key ^ data->params.hash1) % data->nvertex);
	vertex[1] = ((key ^ data->params.hash2) % data->nvertex);
	
	if(vertex[0] == vertex[1])
		return -EBADF;
	
#ifdef MPHF_DEBUG
	printf("mphf: insert: %lx value: %.8lx v:{%lx,%lx:%lx:%lx}\n",
		key, value, vertex[0], vertex[1], data->params.hash1, data->params.hash2
	);
#endif
	return graph_add_edge(data, (uintmax_t *)&vertex, value);
} // }}}
ssize_t mphf_chm_imp_update (mphf_t *mphf, uintmax_t key, uintmax_t value){ // {{{
	ssize_t                ret;
	uintmax_t              vertex[2], g_old, g_new, edge_id;
	vertex_list_t          vertex_new;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	if( (ret = chm_imp_check(mphf, WRITEABLE)) < 0)
		return ret;
	
	vertex[0] = ((key ^ data->params.hash1) % data->nvertex);
	vertex[1] = ((key ^ data->params.hash2) % data->nvertex);

#ifdef MPHF_DEBUG
	printf("mphf: update: %lx value: %.8lx v:{%lx,%lx:%lx:%lx}\n",
		key, value, vertex[0], vertex[1], data->params.hash1, data->params.hash2
	);
#endif

	if(vertex[0] == vertex[1])
		goto insert_new;
	
	if(graph_find_edge(data, 2, (uintmax_t *)&vertex, &edge_id) < 0)
		goto insert_new;
	
	if(graph_getg(data, 1, (uintmax_t *)&vertex[1], &g_old) < 0)
		goto insert_new;
	
	g_new = value - g_old;
		
	vertex_new.vertex = vertex[1];
	vertex_new.next   = NULL;
	
	if( (ret = graph_recalc(data, &vertex_new, g_old, g_new, edge_id)) < 0)
		return ret;
		
	if( (ret = graph_setg(data, 1, &vertex[1], &g_new)) < 0)
		return ret;
	
	return 0;
insert_new:
	return mphf_chm_imp_insert(mphf, key, value);
} // }}}
ssize_t mphf_chm_imp_query  (mphf_t *mphf, uintmax_t key, uintmax_t *value){ // {{{
	intmax_t               ret;
	uintmax_t              vertex[2], g[2], result;
	chm_imp_t             *data = (chm_imp_t *)&mphf->data;
	
	if( (ret = chm_imp_check(mphf, 0)) < 0)
		return ret;
	
	vertex[0] = ((key ^ data->params.hash1) % data->nvertex);
	vertex[1] = ((key ^ data->params.hash2) % data->nvertex);
	
	if(vertex[0] == vertex[1])
		return MPHF_QUERY_NOTFOUND;
	
	if(graph_getg(data, 2, (uintmax_t *)&vertex, (uintmax_t *)&g) < 0)
		return MPHF_QUERY_NOTFOUND;
	
	result = ((g[0] + g[1]) & ( ((uintmax_t)1 << data->bi_value) - 1));
	
#ifdef MPHF_DEBUG
	printf("mphf: query %lx value: %.8lx v:{%lx,%lx:%lx:%lx}\n",
		key, result, vertex[0], vertex[1], data->params.hash1, data->params.hash2
	);
#endif
	*value = result;
	return MPHF_QUERY_FOUND;
} // }}}
ssize_t mphf_chm_imp_delete (mphf_t *mphf, uintmax_t key){ // {{{
	return error("delete not supported");
} // }}}

/* Invalid code {{
typedef struct graph_edge_t {
	// public
	uintmax_t  second_vertex;
	uintmax_t  own_vertex_list;
	uintmax_t  frn_vertex_list;
	// private
	uintmax_t  edge_id;
} graph_edge_t;

// gv list: [1 bit; {bit=0 -> second vertex; bit=1 -> edge_id }]
// ge list: [second vertex; own vertex's edge list; foreign vertex's edge list]

static ssize_t graph_new_edge_id(mphf_t *mphf, uintmax_t *new_id){ // {{{
	chm_imp_build_data_t  *build_data;
	chm_imp_gparams_t      gparams;
	
	build_data = mphf->build_data;
	
	if(backend_stdcall_read  (build_data->backend, build_data->gp_offset, &gparams, sizeof(gparams)) < 0)
		return -EFAULT;
	
	*new_id = gparams.last_edge++;
	
	if(backend_stdcall_write (build_data->backend, build_data->gp_offset, &gparams, sizeof(gparams)) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t graph_get_edge(mphf_t *mphf, chm_imp_t *data, size_t nid, uintmax_t *id, graph_edge_t *edge){ // {{{
	uintmax_t               i, t, k, sizeof_edge, vertex_mask, edgebit_mask;
	char                   buffer[3 * sizeof(uintmax_t)];
	chm_imp_build_data_t  *build_data = mphf->build_data;
	
	sizeof_edge  = 3 * data->bt_vertex;
	vertex_mask  = ( 1 << (data->bt_vertex << 3) ) - 1;
	edgebit_mask = ( 1 << data->bi_vertex );
	
	for(i=0; i < nid; i++){
		// read from vertex array
		if(backend_stdcall_read(
			build_data->backend,
			build_data->gv_offset + id[i] * data->bt_gv,
			&buffer,
			data->bt_gv
		) < 0)
			return -EFAULT;
		
		t = *(uintmax_t *)(buffer);
		k = (t & (edgebit_mask - 1));
		
		if( (t & edgebit_mask) == 0){
			edge[i].second_vertex   = k;
			edge[i].edge_id         = __MAX(uintmax_t);
			edge[i].own_vertex_list = 0;
			edge[i].frn_vertex_list = 0;
		}else{
			edge[i].edge_id         = k;
			
			if(backend_stdcall_read(
				build_data->backend,
				build_data->ge_offset + k * sizeof_edge,
				&buffer,
				sizeof_edge
			) < 0)
				return -EFAULT;
			
			edge[i].second_vertex   = (*(uintmax_t *)(buffer                         )) & vertex_mask;
			edge[i].own_vertex_list = (*(uintmax_t *)(buffer +  data->bt_vertex      )) & vertex_mask;
			edge[i].frn_vertex_list = (*(uintmax_t *)(buffer + (data->bt_vertex << 1))) & vertex_mask;
		}
	}
	return 0;
} // }}}
static ssize_t graph_set_edge(mphf_t *mphf, chm_imp_t *data, size_t nid, uintmax_t *id, graph_edge_t *edge){ // {{{
	uintmax_t               i, edge_size;
	char                   buffer[3 * sizeof(uintmax_t)];
	chm_imp_build_data_t  *build_data = mphf->build_data;
	
	edge_size    = 3 * data->bt_vertex;
	
	for(i=0; i < nid; i++){
		if(edge[i].own_vertex_list != 0 || edge[i].frn_vertex_list != 0){

			// create new edge if not exist
			if(edge[i].edge_id == __MAX(uintmax_t)){
				if(graph_new_edge_id(data, &edge[i].edge_id) < 0)
					return -EFAULT;
			}
			
			*(uintmax_t *)(buffer                         ) = edge[i].second_vertex;
			*(uintmax_t *)(buffer +  data->bt_vertex      ) = edge[i].own_vertex_list;
			*(uintmax_t *)(buffer + (data->bt_vertex << 1)) = edge[i].frn_vertex_list;
			
			if(backend_stdcall_write(
				build_data->backend,
				build_data->ge_offset + edge[i].edge_id * edge_size,
				&buffer,
				edge_size
			) < 0)
				return -EFAULT;
			
			*(uintmax_t *)(buffer)  = edge[i].edge_id;
			*(uintmax_t *)(buffer) |= ( 1 << data->bi_vertex );
		}else{
			*(uintmax_t *)(buffer)  = edge[i].second_vertex;
		}
		
		// write to vertex array
		if(backend_stdcall_write(
			build_data->backend,
			build_data->gv_offset + id[i] * data->bt_gv,
			&buffer,
			data->bt_gv
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_getg(mphf_t *mphf, chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *g){ // {{{
	size_t i;
	
	for(i=0; i<nvertex; i++){
		g[i] = 0;
		
		if(backend_stdcall_read(
			mphf->backend,
			sizeof(data->params) + vertex[i] * data->bt_value,
			&g[i],
			data->bt_value
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_setg(mphf_t *mphf, chm_imp_t *data, size_t nvertex, uintmax_t *vertex, uintmax_t *new_g){ // {{{
	size_t i;

	for(i=0; i<nvertex; i++){
		if(backend_stdcall_write(
			mphf->backend,
			sizeof(data->params) + vertex[i] * data->bt_value,
			&new_g[i],
			data->bt_value
		) < 0)
			return -EFAULT;
	}
	return 0;
} // }}}
static ssize_t graph_recalc(mphf_t *mphf, chm_imp_t *data, uintmax_t initial_vertex, uintmax_t from_vertex, graph_edge_t *parent, uintmax_t g_old, uintmax_t g_new){ // {{{
	uintmax_t       child_vertex, child_g_old, child_g_new;
	graph_edge_t   child;

	child_vertex = parent->own_vertex_list;
	printf("parent: %llx -> { %llx, %llx, %llx }\n", from_vertex, parent->second_vertex, parent->own_vertex_list, parent->frn_vertex_list);
	while(child_vertex != 0){
		if(child_vertex == from_vertex)
			return 0;
		printf("%llx <> %llx\n", child_vertex, from_vertex);

		//if(child_vertex == initial_vertex)
		//	return -EBADF;
		
		if(graph_get_edge(data, data, 1, &child_vertex, &child) < 0)
			return -EFAULT;
		
		printf("child:  %llx -> { %llx, %llx, %llx }\n", child_vertex, child.second_vertex, child.own_vertex_list, child.frn_vertex_list);
		
		if(graph_getg(data, data, 1, &child.second_vertex, &child_g_old) < 0)
			return -EFAULT;
		
		child_g_new = g_old + child_g_old - g_new;
		
		if(graph_recalc(data, data, initial_vertex, child.own_vertex_list, (graph_edge_t *)&child, child_g_old, child_g_new) < 0)
			return -EFAULT;
		
		if(graph_setg(data, data, 1, &child.second_vertex, &child_g_new) < 0)
			return -EFAULT;
		
		child_vertex = child.frn_vertex_list;
		printf("next\n");
	}
	return 0;
} // }}}
static ssize_t graph_add_edge(mphf_t *mphf, chm_imp_t *data, uintmax_t *vertex, uintmax_t value){ // {{{
	ssize_t        ret;
	graph_edge_t   edges[2];
	
	if(graph_get_edge(data, data, 2, vertex, (graph_edge_t *)&edges) < 0){
		printf("get_edge failed\n");
		return -EFAULT;
	}
	
	edges[0].frn_vertex_list = edges[1].own_vertex_list = edges[1].second_vertex;
	edges[1].frn_vertex_list = edges[0].own_vertex_list = edges[0].second_vertex;
	edges[0].second_vertex   = vertex[1];
	edges[1].second_vertex   = vertex[0];
	
	if(graph_set_edge(data, data, 2, vertex, (graph_edge_t *)&edges) < 0){
		printf("set_edge failed\n");
		return -EFAULT;
	}
	
	uintmax_t g[2], g_new, g_free = __MAX(uintmax_t);
	
	if(edges[0].own_vertex_list == 0){
		g_free = 0;
		g[0]   = 0;
	}else{
		if(graph_getg(data, data, 1, &vertex[0], &g[0]) < 0)
			return -EFAULT;
	}
	
	if(edges[1].own_vertex_list == 0){
		g_free = 1;
		g[1]   = 0;
	}else{
		if(graph_getg(data, data, 1, &vertex[1], &g[1]) < 0)
			return -EFAULT;
	}
	
	if(g_free == __MAX(uintmax_t)){
		g_free = 1;
		g_new  = value - g[0]; // g_new
		
		if( (ret = graph_recalc(data, data, vertex[1], vertex[1], &edges[1], g[1], g_new)) < 0)
			return ret;
	}else{
		g_new = value - (g[0] + g[1]);
	}
	
	if(graph_setg(data, data, 1, &vertex[g_free], &g_new) < 0)
		return -EFAULT;
	
	return 0;
} // }}}
}} */
