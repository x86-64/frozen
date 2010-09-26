#include <libfrozen.h>

static int  struct_iter_init(void *p_setting, void *p_structure, void *null){
	size_t    member_id;
	data_type member_data_type;
	char *    member_data_type_s;
	char *    member_name;
	member_t *members;
	
	member_name        = setting_get_child_string((setting_t *)p_setting, "name");
	member_data_type_s = setting_get_child_string((setting_t *)p_setting, "type");
	member_data_type   = data_type_from_string(member_data_type_s);
	
	if(member_name == NULL || member_data_type_s == NULL || member_data_type < 0)
		return ITER_BREAK;
	
	struct_t *structure = (struct_t *)p_structure;
	
	members             = structure->members;
	member_id           = structure->members_count++;
	members = (member_t *)realloc(members, structure->members_count * sizeof(member_t));
	structure->members  = members;
	
	members[member_id].name = member_name;
	data_init(&(members[member_id].data), member_data_type);
	
	structure->size += members[member_id].data.size;
	
	return ITER_CONTINUE;
}

struct_t *  struct_new                   (setting_t *config){
	int       ret;
	struct_t *structure = (struct_t *)malloc(sizeof(struct_t));
	
	memset(structure, 0, sizeof(struct_t));
	
	ret = setting_iter_child(config, (iter_callback)&struct_iter_init, structure, NULL);
	if(ret == ITER_BROKEN){
		struct_free(structure);
		return NULL;
	}
	
	return structure;
}

void        struct_free                  (struct_t *structure){
	if(structure->members != NULL)
		free(structure->members);
	free(structure);
}

member_t *  struct_get_member_by_name    (struct_t *structure, char *name){
	size_t     member_id;
	member_t * members;
	
	members = structure->members;
	for(member_id=0; member_id<structure->members_count; member_id++){
		if(strcmp(members[member_id].name, name) == 0)
			return &(members[member_id]);
	}
	return NULL;
}

