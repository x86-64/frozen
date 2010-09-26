#define DATA_TYPE_U8     1
#define DATA_TYPE_U32    2
#define DATA_TYPE_U64    3
#define DATA_TYPE_STRING 4
#define DATA_TYPE_BINARY 5

unsigned long data_packed_len(int data_type, char *data, unsigned long data_len);
unsigned long data_unpacked_len(int data_type, char *data, unsigned long data_len);
void          data_pack(int data_type, char *string, char *data, unsigned long data_len);
void          data_unpack(int data_type, char *data, unsigned long data_len, char *string);
unsigned int  data_cmp_binary(int data_type, char *data1, char *data2);
