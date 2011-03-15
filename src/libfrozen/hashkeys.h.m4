typedef uint32_t hash_key_t;

m4_define(`CURR_KEY', 1)
m4_define(`REGISTER_KEY', `
        m4_define(`KEYS_ARRAY',  m4_defn(`KEYS_ARRAY')
	`{ "$1", 'CURR_KEY()` },
	')
        m4_define(`KDEFS_ARRAY', m4_defn(`KDEFS_ARRAY')
	`#define HASH_KEY_$1 'CURR_KEY()`
	')
	m4_define(`CURR_KEY', m4_incr(CURR_KEY()))
')

REGISTER_KEY(action)
REGISTER_KEY(backend)
REGISTER_KEY(block_off)
REGISTER_KEY(block_size)
REGISTER_KEY(block_vid)
REGISTER_KEY(blocks)
REGISTER_KEY(buffer)
REGISTER_KEY(buffer_size)
REGISTER_KEY(build_start)
REGISTER_KEY(build_end)
REGISTER_KEY(chains)
REGISTER_KEY(count)
REGISTER_KEY(direction)

// TODO IMPORTANT remove it
REGISTER_KEY(dns_domain)
REGISTER_KEY(dns_ip)
REGISTER_KEY(dns_tstamp)

REGISTER_KEY(engine)
REGISTER_KEY(field)
REGISTER_KEY(file)
REGISTER_KEY(filename)
REGISTER_KEY(forced)
REGISTER_KEY(function)
REGISTER_KEY(handle)
REGISTER_KEY(hash)
REGISTER_KEY(homedir)
REGISTER_KEY(insert)
REGISTER_KEY(item_size)
REGISTER_KEY(key)
REGISTER_KEY(key1)
REGISTER_KEY(key2)
REGISTER_KEY(key3)
REGISTER_KEY(key4)
REGISTER_KEY(key5)
REGISTER_KEY(key_bits)
REGISTER_KEY(key_from)
REGISTER_KEY(key_out)
REGISTER_KEY(key_to)
REGISTER_KEY(keyid)
REGISTER_KEY(list)
REGISTER_KEY(max_size)
REGISTER_KEY(multiply)
REGISTER_KEY(n_initial)
REGISTER_KEY(name)
REGISTER_KEY(nelements)
REGISTER_KEY(offset)
REGISTER_KEY(offset_from)
REGISTER_KEY(offset_out)
REGISTER_KEY(offset_to)
REGISTER_KEY(param)
REGISTER_KEY(path)
REGISTER_KEY(perlevel)
REGISTER_KEY(persistent)
REGISTER_KEY(read_size)
REGISTER_KEY(real_offset)
REGISTER_KEY(rewrite)
REGISTER_KEY(role)
REGISTER_KEY(script)
REGISTER_KEY(size)
REGISTER_KEY(size_old)
REGISTER_KEY(structure)
REGISTER_KEY(type)
REGISTER_KEY(value_bits)
REGISTER_KEY(values)

#ifdef HASH_C
typedef struct hash_keypair_t {
	char *       key_str;
	hash_key_t   key_val;
} hash_keypair_t;

hash_keypair_t  hash_keys[] = {
	KEYS_ARRAY()
};
#define hash_keys_size      sizeof(hash_keys[0])
#define hash_keys_nelements (sizeof(hash_keys)/hash_keys_size)
#endif

KDEFS_ARRAY()

#define HK(_value) HASH_KEY_##_value

