/* Autogenerated file. Don't edit */
#ifndef ACTIONS_H
#define ACTIONS_H
#define ACT ION(value)         ACTION_VALUE_##value
#define ACTION_VALUE_ACQUIRE 0
#define ACTION_VALUE_ADD 1
#define ACTION_VALUE_BATCH 2
#define ACTION_VALUE_COMPARE 3
#define ACTION_VALUE_CONTROL 4
#define ACTION_VALUE_CONVERT_FROM 5
#define ACTION_VALUE_CONVERT_TO 6
#define ACTION_VALUE_CREATE 7
#define ACTION_VALUE_DECREMENT 8
#define ACTION_VALUE_DELETE 9
#define ACTION_VALUE_DIVIDE 10
#define ACTION_VALUE_ENUM 11
#define ACTION_VALUE_EXECUTE 12
#define ACTION_VALUE_FREE 13
#define ACTION_VALUE_GETDATA 14
#define ACTION_VALUE_GETDATAPTR 15
#define ACTION_VALUE_INCREMENT 16
#define ACTION_VALUE_IS_NULL 17
#define ACTION_VALUE_LENGTH 18
#define ACTION_VALUE_LOOKUP 19
#define ACTION_VALUE_MULTIPLY 20
#define ACTION_VALUE_POP 21
#define ACTION_VALUE_PUSH 22
#define ACTION_VALUE_QUERY 23
#define ACTION_VALUE_READ 24
#define ACTION_VALUE_RESIZE 25
#define ACTION_VALUE_START 26
#define ACTION_VALUE_STOP 27
#define ACTION_VALUE_SUB 28
#define ACTION_VALUE_UPDATE 29
#define ACTION_VALUE_WRITE 30
typedef enum action_t {
      ACTION_ACQUIRE = ACTION_VALUE_ACQUIRE, 
      ACTION_ADD = ACTION_VALUE_ADD, 
      ACTION_BATCH = ACTION_VALUE_BATCH, 
      ACTION_COMPARE = ACTION_VALUE_COMPARE, 
      ACTION_CONTROL = ACTION_VALUE_CONTROL, 
      ACTION_CONVERT_FROM = ACTION_VALUE_CONVERT_FROM, 
      ACTION_CONVERT_TO = ACTION_VALUE_CONVERT_TO, 
      ACTION_CREATE = ACTION_VALUE_CREATE, 
      ACTION_DECREMENT = ACTION_VALUE_DECREMENT, 
      ACTION_DELETE = ACTION_VALUE_DELETE, 
      ACTION_DIVIDE = ACTION_VALUE_DIVIDE, 
      ACTION_ENUM = ACTION_VALUE_ENUM, 
      ACTION_EXECUTE = ACTION_VALUE_EXECUTE, 
      ACTION_FREE = ACTION_VALUE_FREE, 
      ACTION_GETDATA = ACTION_VALUE_GETDATA, 
      ACTION_GETDATAPTR = ACTION_VALUE_GETDATAPTR, 
      ACTION_INCREMENT = ACTION_VALUE_INCREMENT, 
      ACTION_IS_NULL = ACTION_VALUE_IS_NULL, 
      ACTION_LENGTH = ACTION_VALUE_LENGTH, 
      ACTION_LOOKUP = ACTION_VALUE_LOOKUP, 
      ACTION_MULTIPLY = ACTION_VALUE_MULTIPLY, 
      ACTION_POP = ACTION_VALUE_POP, 
      ACTION_PUSH = ACTION_VALUE_PUSH, 
      ACTION_QUERY = ACTION_VALUE_QUERY, 
      ACTION_READ = ACTION_VALUE_READ, 
      ACTION_RESIZE = ACTION_VALUE_RESIZE, 
      ACTION_START = ACTION_VALUE_START, 
      ACTION_STOP = ACTION_VALUE_STOP, 
      ACTION_SUB = ACTION_VALUE_SUB, 
      ACTION_UPDATE = ACTION_VALUE_UPDATE, 
      ACTION_WRITE = ACTION_VALUE_WRITE, 
      ACTION_INVALID = 31
} action_t;
#endif

#ifdef ACTION_C
keypair_t actions[] = {
      { "ACQUIRE", ACTION_VALUE_ACQUIRE }, 
      { "ADD", ACTION_VALUE_ADD }, 
      { "BATCH", ACTION_VALUE_BATCH }, 
      { "COMPARE", ACTION_VALUE_COMPARE }, 
      { "CONTROL", ACTION_VALUE_CONTROL }, 
      { "CONVERT_FROM", ACTION_VALUE_CONVERT_FROM }, 
      { "CONVERT_TO", ACTION_VALUE_CONVERT_TO }, 
      { "CREATE", ACTION_VALUE_CREATE }, 
      { "DECREMENT", ACTION_VALUE_DECREMENT }, 
      { "DELETE", ACTION_VALUE_DELETE }, 
      { "DIVIDE", ACTION_VALUE_DIVIDE }, 
      { "ENUM", ACTION_VALUE_ENUM }, 
      { "EXECUTE", ACTION_VALUE_EXECUTE }, 
      { "FREE", ACTION_VALUE_FREE }, 
      { "GETDATA", ACTION_VALUE_GETDATA }, 
      { "GETDATAPTR", ACTION_VALUE_GETDATAPTR }, 
      { "INCREMENT", ACTION_VALUE_INCREMENT }, 
      { "IS_NULL", ACTION_VALUE_IS_NULL }, 
      { "LENGTH", ACTION_VALUE_LENGTH }, 
      { "LOOKUP", ACTION_VALUE_LOOKUP }, 
      { "MULTIPLY", ACTION_VALUE_MULTIPLY }, 
      { "POP", ACTION_VALUE_POP }, 
      { "PUSH", ACTION_VALUE_PUSH }, 
      { "QUERY", ACTION_VALUE_QUERY }, 
      { "READ", ACTION_VALUE_READ }, 
      { "RESIZE", ACTION_VALUE_RESIZE }, 
      { "START", ACTION_VALUE_START }, 
      { "STOP", ACTION_VALUE_STOP }, 
      { "SUB", ACTION_VALUE_SUB }, 
      { "UPDATE", ACTION_VALUE_UPDATE }, 
      { "WRITE", ACTION_VALUE_WRITE }, 
      { NULL, 0 }  
};
#endif
