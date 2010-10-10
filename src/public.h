#ifndef PUBLIC_H
#define PUBLIC_H

typedef enum request_action {
	ACTION_CRWD_CREATE,
	ACTION_CRWD_READ,
	ACTION_CRWD_WRITE,
	ACTION_CRWD_DELETE,
	ACTION_CRWD_MOVE,
	ACTION_CRWD_COUNT,
	ACTION_CRWD_CUSTOM
} request_action;

typedef hash_t  request_t;

#endif // PUBLIC_H
