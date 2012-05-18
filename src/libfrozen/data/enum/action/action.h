/* Autogenerated file. Don't edit */
#ifndef ACTIONS_H
#define ACTIONS_H
#define ACT ION(value)         ACTION_VALUE_##value
#define ACTION_VALUE_ADD 0
#define ACTION_VALUE_BATCH 1
#define ACTION_VALUE_COMPARE 2
#define ACTION_VALUE_CONSUME 3
#define ACTION_VALUE_CONTROL 4
#define ACTION_VALUE_CONVERT_FROM 5
#define ACTION_VALUE_CONVERT_TO 6
#define ACTION_VALUE_CREATE 7
#define ACTION_VALUE_DECREMENT 8
#define ACTION_VALUE_DELETE 9
#define ACTION_VALUE_DIVIDE 10
#define ACTION_VALUE_ENUM 11
#define ACTION_VALUE_FREE 12
#define ACTION_VALUE_INCREMENT 13
#define ACTION_VALUE_IS_NULL 14
#define ACTION_VALUE_LENGTH 15
#define ACTION_VALUE_LOOKUP 16
#define ACTION_VALUE_MULTIPLY 17
#define ACTION_VALUE_POP 18
#define ACTION_VALUE_PUSH 19
#define ACTION_VALUE_QUERY 20
#define ACTION_VALUE_READ 21
#define ACTION_VALUE_RESIZE 22
#define ACTION_VALUE_SUB 23
#define ACTION_VALUE_UPDATE 24
#define ACTION_VALUE_VIEW 25
#define ACTION_VALUE_WRITE 26
typedef enum action_t {
      ACTION_ADD = ACTION_VALUE_ADD, 
      ACTION_BATCH = ACTION_VALUE_BATCH, 
      ACTION_COMPARE = ACTION_VALUE_COMPARE, 
      ACTION_CONSUME = ACTION_VALUE_CONSUME, 
      ACTION_CONTROL = ACTION_VALUE_CONTROL, 
      ACTION_CONVERT_FROM = ACTION_VALUE_CONVERT_FROM, 
      ACTION_CONVERT_TO = ACTION_VALUE_CONVERT_TO, 
      ACTION_CREATE = ACTION_VALUE_CREATE, 
      ACTION_DECREMENT = ACTION_VALUE_DECREMENT, 
      ACTION_DELETE = ACTION_VALUE_DELETE, 
      ACTION_DIVIDE = ACTION_VALUE_DIVIDE, 
      ACTION_ENUM = ACTION_VALUE_ENUM, 
      ACTION_FREE = ACTION_VALUE_FREE, 
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
      ACTION_SUB = ACTION_VALUE_SUB, 
      ACTION_UPDATE = ACTION_VALUE_UPDATE, 
      ACTION_VIEW = ACTION_VALUE_VIEW, 
      ACTION_WRITE = ACTION_VALUE_WRITE, 
      ACTION_INVALID = 27
} action_t;
#endif

#ifdef ACTION_C
keypair_t actions[] = {
      { "ADD", ACTION_VALUE_ADD }, 
      { "BATCH", ACTION_VALUE_BATCH }, 
      { "COMPARE", ACTION_VALUE_COMPARE }, 
      { "CONSUME", ACTION_VALUE_CONSUME }, 
      { "CONTROL", ACTION_VALUE_CONTROL }, 
      { "CONVERT_FROM", ACTION_VALUE_CONVERT_FROM }, 
      { "CONVERT_TO", ACTION_VALUE_CONVERT_TO }, 
      { "CREATE", ACTION_VALUE_CREATE }, 
      { "DECREMENT", ACTION_VALUE_DECREMENT }, 
      { "DELETE", ACTION_VALUE_DELETE }, 
      { "DIVIDE", ACTION_VALUE_DIVIDE }, 
      { "ENUM", ACTION_VALUE_ENUM }, 
      { "FREE", ACTION_VALUE_FREE }, 
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
      { "SUB", ACTION_VALUE_SUB }, 
      { "UPDATE", ACTION_VALUE_UPDATE }, 
      { "VIEW", ACTION_VALUE_VIEW }, 
      { "WRITE", ACTION_VALUE_WRITE }, 
      { NULL, 0 }  
};
#endif
