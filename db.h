typedef struct dbtable_ dbtable;
struct dbtable_ {
	char            *name;
	
	/* table settings */
	int              data_type;
	
	/* table data */
	dbmap            data;
	dbindex          idx_oid;
	dbindex          idx_data;
};

typedef struct dbitem_ {
    unsigned long  oid;
    char*          attribute;
    char*          query;
    char*          data;
    unsigned long  data_len;
    dbtable       *table;
} dbitem;

dbtable*      db_tables_search(char *name);

dbitem*       dbitem_alloc(void);
void          dbitem_append(dbitem *item, void *data, unsigned long data_len);
void          dbitem_free(dbitem*);

unsigned long db_query_new(void);
unsigned int  db_query_set(dbitem *item);
unsigned int  db_query_get(dbitem *item);
unsigned int  db_query_delete(dbitem *item);
unsigned int  db_query_search(dbitem *item);
unsigned int  db_query_init(dbtable *table, int type);
unsigned int  db_query_deindex(dbtable *table);
unsigned int  db_query_index(dbtable *table, int type);
unsigned int  db_query_destroy(dbtable *table);

void          db_init(void);
void          db_destroy(void);

