include(zeromq.m4)

include(article_flow_config.m4)

 

{ class = "assign", before = {                                             // set our data
	data = (raw_t)"client_data"
}},

ZEROMQ_HASH_PUSH(`SERVICE_A_connect', `                                    // push our request
	destination = ZEROMQ_LAZY_SOCKET(`push', `SERVICE_B_connect'),     // - our destination socket - it is service B
	data        = (env_t)"data"                                        // - our data
'),
{ class = "end" }
