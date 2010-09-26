#include "test.h"

START_TEST (test_structs){
	setting_t *settings = setting_new();
		setting_t *s_d1 = setting_new();
			setting_set_child_string(s_d1, "name",     "data1");
			setting_set_child_string(s_d1, "type",     "int32");
		setting_t *s_d2 = setting_new();
			setting_set_child_string(s_d2, "name",     "data2");
			setting_set_child_string(s_d2, "type",     "int32");
	setting_set_child_setting(settings, s_d2);
	setting_set_child_setting(settings, s_d1);
	
	struct_t *structure = struct_new(settings);
		fail_unless(structure != NULL, "structure create failed");
	member_t *m_d1 = struct_get_member_by_name(structure, "data1");
	member_t *m_d2 = struct_get_member_by_name(structure, "data2");
		fail_unless(m_d1 != NULL && m_d2 != NULL, "member search failed");
		fail_unless(m_d1 != m_d2,                 "member search invalid");
		fail_unless(m_d1->data.size == 4,         "member size invalid");
	
	size_t size = struct_get_size(structure);
		fail_unless(size == 8,                    "structure size invalid");
	
	struct_free(structure);
}
END_TEST
REGISTER_TEST(core, test_structs)

