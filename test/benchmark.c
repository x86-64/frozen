#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>

#include <libfrozen.h>

/* typedefs */
void opts_help(void);

/* global options */
unsigned int    opt_verbose        = 0;

/* getopt arrays */
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

static struct cmdline_option option_data[] = { 
	{ "help",         'h', OPT_FUNC,       (void *)&opts_help,    "help screen"                                                          },
	{ "verbose",      'v', OPT_VALUE_BOOL, &opt_verbose,          "verbose"                                                              },
	{ NULL, 0, 0, NULL, NULL }
};

static struct option            long_options  [sizeof(option_data)/sizeof(struct cmdline_option) + 1]; 
static char                     short_options [sizeof(option_data)/sizeof(struct cmdline_option) + 1]; 
static struct cmdline_option *  map_opts      [96]; 


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

int main (int argc, char **argv){
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
	
	
	
	return 0;
}
