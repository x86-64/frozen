
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libfrozen.h>

/* global options */
char            defopt_write_file[] = "benchmark_baseline.dat";

unsigned int    opt_verbose         = 0;
unsigned int    opt_iters           = 10000;
char           *opt_write_file      = defopt_write_file;
unsigned int    opt_item_size       = 10;

/* getopt arrays  {{{ */
struct cmdline_option {
	const char *long_name;
	char        short_name;
	enum {
		OPT_VALUE_INT,
		OPT_VALUE_STR,
		OPT_VALUE_BOOL,
		OPT_FUNC,
		OPT_GROUP
	} type; 
	
	void       *opt_ptr;
	const char *helpmsg;
}; 

typedef void (*opt_func)(void);
void opts_help(void);

static struct cmdline_option option_data[] = { 
	{ "iters",        'i', OPT_VALUE_INT,  &opt_iters,            "number of iterations for each test, def 10000"                        },
	{ "help",         'h', OPT_FUNC,       (void *)&opts_help,    "help screen"                                                          },
	{ "verbose",      'v', OPT_VALUE_BOOL, &opt_verbose,          "verbose"                                                              },
	{ "Test 'baseline':", 0, OPT_GROUP,    NULL, NULL                                                                                    },
	{ "file",         'f', OPT_VALUE_STR,  &opt_write_file,       "test_baseline file to write"                                          },
	{ "size",         's', OPT_VALUE_INT,  &opt_item_size,        "test_baseline read\\write chunk size"                                 },
	{ NULL, 0, 0, NULL, NULL }
};

static struct option            long_options  [sizeof(option_data)/sizeof(struct cmdline_option) + 1]; 
static char                     short_options [sizeof(option_data)/sizeof(struct cmdline_option) + 1]; 
static struct cmdline_option *  map_opts      [96]; 
/* }}} */
char           *item;

/* benchmarks {{{ */
typedef struct bench_t {
	struct timeval  tv_start;
	struct timeval  tv_end;
	struct timeval  tv_diff;
	unsigned long   time_ms;
	unsigned long   time_us;
	char            string[255];
} bench_t;

void timeval_subtract (result, x, y)
        struct timeval *result, *x, *y; 
{
    
        if (x->tv_usec < y->tv_usec) {
                int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
                y->tv_usec -= 1000000 * nsec;
                y->tv_sec += nsec;
        }   
        if (x->tv_usec - y->tv_usec > 1000000) {
                int nsec = (x->tv_usec - y->tv_usec) / 1000000;
                y->tv_usec += 1000000 * nsec;
                y->tv_sec -= nsec;
        }   
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;
}


bench_t * bench_start(void){
        bench_t *bench = calloc(sizeof(bench_t), 1); 
        gettimeofday(&bench->tv_start, NULL);
        return bench; 
}
void bench_free(bench_t *bench){
	free(bench);
}
void bench_query(bench_t *bench){
	gettimeofday(&bench->tv_end, NULL);
    
        timeval_subtract(&bench->tv_diff, &bench->tv_end, &bench->tv_start);
        sprintf(bench->string, "%u.%.6u", (unsigned int)bench->tv_diff.tv_sec, (unsigned int)bench->tv_diff.tv_usec);
	
	bench->time_ms = bench->tv_diff.tv_usec / 1000  + bench->tv_diff.tv_sec * 1000;
	bench->time_us = bench->tv_diff.tv_usec         + bench->tv_diff.tv_sec * 1000000;
}
void bench_func(char *name, int (*func)(unsigned int), unsigned int iters){
	int       ret;
	bench_t  *be = bench_start();
	
	ret = func(iters);
	
	bench_query(be);
	printf("%u iters took %s sec; speed: %7lu iters/s; one iter: %5lu ms/ %10lu us   benchmark of '%s' %s\n",
		iters,
		be->string,
		( iters * 1000 / be->time_ms), 
		( be->time_ms / iters ),
		( be->time_us / iters ),
		name,
		( ret == 0 ) ? "succeed" : "FAILED" 
	);
	
	bench_free(be);
}
/* }}} */

int test_baseline_seqwrite(unsigned int iters){ // {{{
	int          handle;
	off_t        off;
	ssize_t      ret;
	
	handle = open(
		opt_write_file,
		O_CREAT | O_RDWR,
		S_IRUSR | S_IWUSR
	);
	
	if(handle == -1)
		return -1;
	
	off = 0;
	while(iters > 0){
		lseek(handle, off, SEEK_SET);
		ret = write(
				handle,
				item,
				opt_item_size
			);
		if(ret == -1)
			return -1;
		
		off  += opt_item_size;
		iters--;
	}	
	close(handle);
	return 0;
} // }}}
int test_baseline_seqread(unsigned int iters){ // {{{
	int          handle;
	off_t        off;
	ssize_t      ret;
	void        *buff;
	
	buff = malloc(opt_item_size);
	
	handle = open(
		opt_write_file,
		O_CREAT | O_RDWR,
		S_IRUSR | S_IWUSR
	);
	
	if(handle == -1)
		return -1;
	
	off = 0;
	while(iters > 0){
		lseek(handle, off, SEEK_SET);
		ret = read(
				handle,
				buff,
				opt_item_size
			);
		if(ret == -1)
			goto cleanup;
		
		off  += opt_item_size;
		iters--;
	}	
	close(handle);
	ret = 0;

cleanup:
	free(buff);
	return (int)ret;
} // }}}

void main_cleanup(void){
	/* cleanup */
	free(item);
	unlink(opt_write_file);
}

void main_rest(void){
	/* options init */
	item          = malloc(opt_item_size);
	memset(item, 'A', opt_item_size);
	
	/* tests */
	bench_func("baseline_seq_write",   &test_baseline_seqwrite, opt_iters);
	bench_func("baseline_seq_rewrite", &test_baseline_seqwrite, opt_iters);
	bench_func("baseline_seq_read",    &test_baseline_seqread,  opt_iters);
	
}

void opts_init(void){ // {{{
	int                    i;
	struct cmdline_option *mopt = option_data;
	struct option         *lopt = long_options;
	char                  *sopt = short_options;
	
	i = 0;
	while(mopt->long_name != NULL){
		if(mopt->type == OPT_GROUP)
			goto next;
		
		lopt->name       = mopt->long_name;
		lopt->val        = i;
		
		if(mopt->short_name){
			*sopt++                         = mopt->short_name;
			map_opts[mopt->short_name - 32] = mopt;
		}
		
		switch(mopt->type){
			case OPT_VALUE_STR:
			case OPT_VALUE_INT:
				lopt->has_arg = required_argument;
				if(mopt->short_name)
					*sopt++       = ':';
				break;
			case OPT_VALUE_BOOL:
			case OPT_FUNC:
				lopt->has_arg = no_argument;
				break;
			case OPT_GROUP:
				break;
		}
		
		lopt++;
	next:
		mopt++;
		i++;
	}
	*sopt = '\0';
} // }}}
void opts_help(void){ // {{{
	fprintf(stderr, "usage: benchmark [OPTIONS]\n\nOptions:\n");
	struct cmdline_option *mopt = option_data;
	char                  *p;
	char                   col1[25]; 
	
	while(mopt->long_name != NULL){
		if(mopt->type == OPT_GROUP){
			fprintf(stderr, "%s\n", mopt->long_name);
			goto next;
		}

		memset(col1 + 0,  ' ', 25);
		p = col1 + 2;
		memset(p,  '-', 2);                                  p += 2;
		memcpy(p, mopt->long_name, strlen(mopt->long_name)); p += strlen(mopt->long_name);
		if(mopt->short_name){
			memset(p++, ',', 1);
			memset(p++, ' ', 1);
			memset(p++, '-', 1);
			memset(p++, mopt->short_name, 1);
		}
		
		fprintf(stderr, "%.*s%s\n", 25, col1, mopt->helpmsg);
	next:
		mopt++;
	}
	
	exit(255);
} // }}}
int main (int argc, char **argv){ // {{{
	int c;
	int option_index;
	opt_func func;
	
	opts_init();
	
	option_index = -1;
	while( (c = getopt_long (argc, argv, short_options, long_options, &option_index)) != -1){
		struct cmdline_option *mopt;
		
		if(option_index == -1){
			mopt = map_opts[c - 32];
		}else{
			mopt = &option_data[ long_options[option_index].val ];
		}
		if(mopt == NULL)
			exit(255);
		
		switch(mopt->type){
			case OPT_VALUE_INT:
				*(unsigned int *)(mopt->opt_ptr) = atoi(optarg);
				break;
			case OPT_VALUE_STR:
				*(char **)       (mopt->opt_ptr) = strdup(optarg);
				break;
			case OPT_VALUE_BOOL:
				*(unsigned int *)(mopt->opt_ptr) = 1;
				break;
			case OPT_GROUP:
				break;
			case OPT_FUNC:
				func = (opt_func)(mopt->opt_ptr);
				func();
				exit(0);
		}
		
		option_index = -1;
	}
	
	main_cleanup();	
	main_rest();
	main_cleanup();	
	
	return 0;
} // }}}
