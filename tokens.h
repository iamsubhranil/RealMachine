/* All available tokens are defined here.
 * ET or emit token is a multipurpose macro
 * which should be defined before the point
 * where this file is included.
 */

// ,
ET(comma)

// r
ET(register)

// #
ET(hash)

// @
ET(address)

// any consecutive sequence of integers
ET(number)

// end of file marker
ET(eof)

// any unknown token
ET(unknown)

// any consecutive sequence of alphabets, execept keywords
ET(label)

// :
ET(colon)

// any consecutive sequence of characters
ET(string)

#ifdef RM_ALLOW_PARSE_MESSAGES
// any string between { }
ET(parseMessage)
#endif

// keywords
#define OPCODE(x, a, b) ET(x)
#include "opcodes.h"
#undef OPCODE
