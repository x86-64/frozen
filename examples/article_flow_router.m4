include(daemon.m4)
include(zeromq.m4)
include(article_flow_config.m4)

DAEMON(`
	ZEROMQ_DEVICE(`streamer', `SERVICE_A_bind', `SERVICE_A_WORKER_bind')
')

DAEMON(`
	ZEROMQ_DEVICE(`streamer', `SERVICE_B_bind', `SERVICE_B_WORKER_bind')
')

{ class = "end" }
