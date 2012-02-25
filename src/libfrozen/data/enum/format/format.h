/* Autogenerated file. Don't edit */
#ifndef FORMATS_H
#define FORMATS_H
#define FORMAT(value)         FORMAT_VALUE_##value
#define FORMAT_VALUE_clean 5742
#define FORMAT_VALUE_config 9498
#define FORMAT_VALUE_debug 5833
#define FORMAT_VALUE_hash 3191
#define FORMAT_VALUE_human 5855
#define FORMAT_VALUE_ip_dothexint 71753
#define FORMAT_VALUE_ip_dotint 31540
#define FORMAT_VALUE_ip_dotoctint 71644
#define FORMAT_VALUE_ip_hexint 31498
#define FORMAT_VALUE_ip_int 10014
#define FORMAT_VALUE_native 9808
#define FORMAT_VALUE_netstring 30930
#define FORMAT_VALUE_packed 9228
#define FORMAT_VALUE_time_dot_dmy 70279
#define FORMAT_VALUE_time_dot_dmyhm 109219
#define FORMAT_VALUE_time_dot_dmyhms 135094
#define FORMAT_VALUE_time_rfc2822 43846
#define FORMAT_VALUE_time_rfc3339 44556
#define FORMAT_VALUE_time_slash_dmy 108895
#define FORMAT_VALUE_time_slash_dmyhm 160199
#define FORMAT_VALUE_time_slash_dmyhms 193434
#define FORMAT_VALUE_time_unix 31550
#define FORMAT_VALUE_value 5875
typedef enum format_t {
      FORMAT_clean = FORMAT_VALUE_clean, 
      FORMAT_config = FORMAT_VALUE_config, 
      FORMAT_debug = FORMAT_VALUE_debug, 
      FORMAT_hash = FORMAT_VALUE_hash, 
      FORMAT_human = FORMAT_VALUE_human, 
      FORMAT_ip_dothexint = FORMAT_VALUE_ip_dothexint, 
      FORMAT_ip_dotint = FORMAT_VALUE_ip_dotint, 
      FORMAT_ip_dotoctint = FORMAT_VALUE_ip_dotoctint, 
      FORMAT_ip_hexint = FORMAT_VALUE_ip_hexint, 
      FORMAT_ip_int = FORMAT_VALUE_ip_int, 
      FORMAT_native = FORMAT_VALUE_native, 
      FORMAT_netstring = FORMAT_VALUE_netstring, 
      FORMAT_packed = FORMAT_VALUE_packed, 
      FORMAT_time_dot_dmy = FORMAT_VALUE_time_dot_dmy, 
      FORMAT_time_dot_dmyhm = FORMAT_VALUE_time_dot_dmyhm, 
      FORMAT_time_dot_dmyhms = FORMAT_VALUE_time_dot_dmyhms, 
      FORMAT_time_rfc2822 = FORMAT_VALUE_time_rfc2822, 
      FORMAT_time_rfc3339 = FORMAT_VALUE_time_rfc3339, 
      FORMAT_time_slash_dmy = FORMAT_VALUE_time_slash_dmy, 
      FORMAT_time_slash_dmyhm = FORMAT_VALUE_time_slash_dmyhm, 
      FORMAT_time_slash_dmyhms = FORMAT_VALUE_time_slash_dmyhms, 
      FORMAT_time_unix = FORMAT_VALUE_time_unix, 
      FORMAT_value = FORMAT_VALUE_value, 
} format_t;
#endif

#ifdef FORMAT_C
keypair_t formats[] = {
      { "clean", FORMAT_VALUE_clean }, 
      { "config", FORMAT_VALUE_config }, 
      { "debug", FORMAT_VALUE_debug }, 
      { "hash", FORMAT_VALUE_hash }, 
      { "human", FORMAT_VALUE_human }, 
      { "ip_dothexint", FORMAT_VALUE_ip_dothexint }, 
      { "ip_dotint", FORMAT_VALUE_ip_dotint }, 
      { "ip_dotoctint", FORMAT_VALUE_ip_dotoctint }, 
      { "ip_hexint", FORMAT_VALUE_ip_hexint }, 
      { "ip_int", FORMAT_VALUE_ip_int }, 
      { "native", FORMAT_VALUE_native }, 
      { "netstring", FORMAT_VALUE_netstring }, 
      { "packed", FORMAT_VALUE_packed }, 
      { "time_dot_dmy", FORMAT_VALUE_time_dot_dmy }, 
      { "time_dot_dmyhm", FORMAT_VALUE_time_dot_dmyhm }, 
      { "time_dot_dmyhms", FORMAT_VALUE_time_dot_dmyhms }, 
      { "time_rfc2822", FORMAT_VALUE_time_rfc2822 }, 
      { "time_rfc3339", FORMAT_VALUE_time_rfc3339 }, 
      { "time_slash_dmy", FORMAT_VALUE_time_slash_dmy }, 
      { "time_slash_dmyhm", FORMAT_VALUE_time_slash_dmyhm }, 
      { "time_slash_dmyhms", FORMAT_VALUE_time_slash_dmyhms }, 
      { "time_unix", FORMAT_VALUE_time_unix }, 
      { "value", FORMAT_VALUE_value }, 
      { NULL, 0 }  
};
#endif
