#include <libfrozen.h>

// TODO rewrite to handle buffer having data splitted between two chunks

int         struct_get_member_value      (buffer_t *buffer, unsigned int member_id, subchunk_t *chunk){
	unsigned int   c;
	void          *d_ptr   = NULL;
	off_t          m_ptr   = 0;
	member_t      *members = buffer->structure->members;
	
	if(member_id >= buffer->structure->members_count)
		return -EINVAL;
	
	m_ptr = members[member_id].ptr;
	
	if(m_ptr == 0 && member_id != 0){
		//c = member_id - 1                        // TODO uncomment to test speedup
		//while(c != 0 && members[c].ptr == NULL)  // scroll back to known pointer
		//	c--;
		//m_ptr = members[c].ptr;
		
		c = 0;
		while(c < member_id){
			d_ptr  = buffer_seek(buffer, m_ptr);
			m_ptr += data_len(members[c].data_type, d_ptr);
			
			// TODO add ptr's cache 
			c++;
		}
	}
	
	chunk->ptr  = buffer_seek(buffer, m_ptr);
	chunk->size = data_len(members[member_id].data_type, chunk->ptr);
	
	return 0;
}

