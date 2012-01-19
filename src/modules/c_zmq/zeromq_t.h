#ifndef DATA_ZEROMQT_H
#define DATA_ZEROMQT_H

/**
 * @ingroup data
 * @addtogroup zeromq_t zeromq_t
 */
/**
 * @ingroup zeromq_t
 * @page page_zmq_info Description
 *
 * This module implement stubs to work with ZeroMQ.
 */
/**
 * @ingroup zeromq_t
 * @page page_zmq_config Configuration
 * 
 * Accepted configuration:
 * @code
 * some = (zeromq_t){
 *              type                    =                     # socket type to create, see man zmq_socket
 *                                        "req",
 *                                        "rep",
 *                                        "dealer",
 *                                        "router",
 *                                        "pub",
 *                                        "sub",
 *                                        "push",
 *                                        "pull",
 *                                        "pair",
 *              identity                = "ident",            # set socket identity
 *              bind                    = "tcp://",           # bind socket to            OR
 *              connect                 = "tcp://".           # connect socket to
 * }
 * @endcode
 */

#define ZEROMQT_NAME  "zeromq_t"
#define TYPE_ZEROMQT  data_getid(ZEROMQT_NAME, NULL)

#define DEREF_TYPE_ZEROMQT(_data) (ipv4_t *)((_data)->ptr) 
#define REF_TYPE_ZEROMQT(_dt) _dt 
#define HAVEBUFF_TYPE_ZEROMQT 0

#endif
