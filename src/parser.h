#include <stdlib.h>

struct AlternativeLink;
struct OptionsParserState;

/* parser.c
 * parsing priority options installed on the system
 *
 * format:
 * binary = /some/path
 * man = some_manpage.1
 *
 * space is optional and ignored. Fail on unexpected input.
 */

struct OptionsParserState* initOptionsParser();

// parses some input, can be partial
// return 0 on OK, -1 on fail
int parseOptionsData(const char *buffer, size_t len, struct OptionsParserState *state);

// Frees parsing state. returns array of links from the options, end with ALTLINK_EOL type
// entry. NULL on error.
struct AlternativeLink* doneOptionsParser(int priority, struct OptionsParserState *state);



/* config_parser.c
 * Parsing user override config files which is located in
 * /etc/libalternatives.conf or $HOME/.config/libalternatives.conf
 *
 * format:
 * binary_name=<priority>
 *
 * where
 *    binary_name is passed exactly in the init function.
 *    <priority> is a base-10 number followed by a \n.
 *    Comments starts with #.
 *
 * Space is optional and ignored.
 * Lines that do not match this syntax are to be treated as comments and
 * ignored.
 */

struct ConfigParserState;

struct ConfigParserState* initConfigParser(const char *binary_name);

// parses some input, can be partial
// returns
//   "priority" when match found and is parsed, 0 otherwise
// on further input, returns already matched priority above and ignores
// input as comments
int parseConfigData(const char *buffer, struct ConfigParserState *state);

// frees any memory allocated by parser
void doneConfigParser(struct ConfigParserState *state);

// set or resets priority (clears config entry) for the binary_name
// assumes the entire config is already parsed by parseCondifData()
//    return NULL if state is freed (eg. by the doneConfigParser
//    function)
void setConfigPriority(int priority, struct ConfigParserState *state);
void resetConfigDefaultPriority(struct ConfigParserState *state);
int getConfigLineNr(const struct ConfigParserState *state);
int getConfigPriority(const struct ConfigParserState *state);
const char *getConfigBinaryName(const struct ConfigParserState *state);
