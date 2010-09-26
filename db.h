typedef struct dbtable_ dbtable;
struct dbtable_ {
	char            *name;
	
	/* table settings */
	int              pack_type;
	
	/* table data */
	dbmap            data;
	dbindex          idx_oid;
	dbindex          idx_data;
};

typedef struct dbitem_ {
    unsigned long  oid;
    char*          attribute;
    char*          data;
    unsigned long  data_len;
    dbtable       *table;
} dbitem;

dbtable*      db_tables_search(char *name);

dbitem*       dbitem_alloc(void);
void          dbitem_free(dbitem*);

unsigned long db_query_new(void);
unsigned int  db_query_set(dbitem *);
unsigned int  db_query_search(dbitem *item);

void          db_init(void);
void          db_destroy(void);

