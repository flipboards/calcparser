#pragma once

#define ERROR_BAD_VAR		0x01	// bad variable name
#define ERROR_BAD_FUNC		0x02	// bad function/operator name
#define ERROR_STACK_SYM		0x04	// symbol stack not empty
#define ERROR_STACK_CALL	0x05	// call stack not empty (by other reasons)
#define ERROR_STACK_FRAME	0x06	// frame stack error (too less or too more)
#define ERROR_STACK_OVERFLOW 0x07	// stack overflow
#define ERROR_JUMP			0x0a	// invalid jump address
#define ERROR_NO_MAIN		0x0e	// no main function found
#define ERROR_GRAMMAR_START	0x0f	// grammar error offset
#define ERROR_GRAMMAR_END	0x80
#define ERROR_MATH_START	0x80	// mathmatical error offset
#define ERROR_MATH_END		0xff
#define ERROR_UNDEFINED		0xff

