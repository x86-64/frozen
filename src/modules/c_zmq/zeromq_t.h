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
 *              type                    =                     // socket type to create, see man zmq_socket
 *                                        "req",
 *                                        "rep",
 *                                        "dealer",
 *                                        "router",
 *                                        "pub",
 *                                        "sub",
 *                                        "push",
 *                                        "pull",
 *                                        "pair",
 *              identity                = "ident",            // set socket identity
 *              bind                    = "tcp://",           // bind socket to            OR
 *              connect                 = "tcp://".           // connect socket to
 *              subscribe               = "filter",           // see zmq_setsockopt manual
 *              unsubscribe             = "filter",           // see zmq_setsockopt manual
 *              hwm                     = (uint64_t)"0",      // see zmq_setsockopt manual
 *              affinity                = (uint64_t)"0",      // see zmq_setsockopt manual
 *              sndbuf_size             = (uint64_t)"0",      // see zmq_setsockopt manual
 *              rcvbuf_size             = (uint64_t)"0",      // see zmq_setsockopt manual
 *              swap                    = (int64_t)"0",       // see zmq_setsockopt manual
 *              rate                    = (int64_t)"0",       // see zmq_setsockopt manual
 *              recovery_ivl            = (int64_t)"0",       // see zmq_setsockopt manual
 *              recovery_ivl_msec       = (int64_t)"0",       // see zmq_setsockopt manual
 *              mcast_loop              = (int64_t)"0",       // see zmq_setsockopt manual
 *              linger                  = (int_t)"0",         // see zmq_setsockopt manual
 *              reconnect_ivl           = (int_t)"0",         // see zmq_setsockopt manual
 *              reconnect_ivl_max       = (int_t)"0",         // see zmq_setsockopt manual
 *              backlog                 = (int_t)"0",         // see zmq_setsockopt manual
 *              lazy                    = (uint_t)"1"         // do not create socket until it actually need
 * }
 * @endcode
 *
 * @code
 * some = (zeromq_proxy_t){
 *              // SERVER MODE
 *              data                    = (something_t)""     // slave data to send requests to
 *              ... (same options as for zeromq_t) ...
 *              ... bind or connect here ...
 *
 *              // CLIENT MODE
 *              ... (same options as for zeromq_t) ...
 *              ... bind or connect here ...
 * }
 */
/**
 * @ingroup machine
 * @addtogroup mod_machine_zeromq modules/zeromq
 */
/**
 * @ingroup mod_machine_zeromq
 * @page page_zeromq_info Description
 *
 * This machine help to deal with zeromq sockets using common pattern. 
 */
/**
 * @ingroup mod_machine_zeromq
 * @page page_zeromq_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "modules/zeromq",
 *              socket                  =                        // zeromq socket
 *                                        (env_t)"data",         // - zeromq socket supplied in current request
 *                                        (zeromq_t)"",          // - embedded in configuration
 *              shop                    = (machine_t){ ... },    // shop for processing rep socket's incoming request
 *              input                   = (hashkey_t)"input",    // input hashkey, default "buffer"
 *              output                  = (hashkey_t)"output",   // output hashkey, default "buffer"
 *              convert                 = (uint_t)"1"            // convert input data to raw_t (use for non-consumable data), default 0
 * }
 * @endcode
 */

#define ZEROMQT_NAME       "zeromq_t"
#define ZEROMQ_MSGT_NAME   "zeromq_msg_t"
#define ZEROMQ_PROXYT_NAME "zeromq_proxy_t"

datatype_t                type_zeromqt; 
datatype_t                type_zeromq_msgt;
datatype_t                type_zeromq_proxyt;

#define TYPE_ZEROMQT      type_zeromqt
#define TYPE_ZEROMQ_MSGT  type_zeromq_msgt
#define TYPE_ZEROMQ_PROXYT  type_zeromq_proxyt

#define DATA_PTR_ZEROMQ_MSGT(_msg) { TYPE_ZEROMQ_MSGT, _msg } 
#define DEREF_TYPE_ZEROMQT(_data) (ipv4_t *)((_data)->ptr) 
#define REF_TYPE_ZEROMQT(_dt) _dt 
#define HAVEBUFF_TYPE_ZEROMQT 0

#endif
