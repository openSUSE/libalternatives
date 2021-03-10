#include <stdlib.h>

struct AlternativeLink;
struct OptionsParserState;

/* options_parser.c
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

/** @brief Initialize an ConfigParserState object with a given binary name.
 *
 * @param binary_name Binary name (group name).
 * @return Pointer of an allocated ConfigParserState object.
 */
struct ConfigParserState* initConfigParser(const char *binary_name);

/** @brief Parsing a string for the FIRST entry <binary_name>=<priority>.
 *
 * @param buffer String which has to be parsed.
 * @param state Reference on an ConfigParserState object with which binary_name
 *              has been defined. The priority and line number will be stored
 *              there too if an entry has been found.
 * @return Priority for the given binary name or 0 it it has not been found.
 */
int parseConfigData(const char *buffer, struct ConfigParserState *state);

/** @brief Frees any memory allocated by parser.
 *
 * @param state Reference on an ConfigParserState object which will be freed.
 */
void doneConfigParser(struct ConfigParserState *state);

/** @brief Set priority for a binary name in the given state struct.
 *
 * @param priority Priority which has to be set.
 * @param state Reference on an ConfigParserState object in which the
 *              priority will be set.
 */
void setConfigPriority(int  priority, struct ConfigParserState *state);

/** @brief Set default priority for a binary name in the given state struct.
 *
 * @param state Reference on an ConfigParserState object in which the
 *              priority will be set.
 */
void setConfigDefaultPriority(struct ConfigParserState *state);

/** @brief Get the line number where the binary_name priority has been parsed.
 *
 * @param state Reference on an ConfigParserState object in which the
 *              line number is set.
 * @return line number
 */
int getConfigLineNr(const struct ConfigParserState *state);

/** @brief Get priority for a binary name in the given state struct.
 *
 * @param state Reference on an ConfigParserState object in which the
 *              priority is set.
 * @return priority
 */
int getConfigPriority(const struct ConfigParserState *state);

/** @brief Get binary name of the given state struct.
 *
 * @param state Reference on an ConfigParserState object in which the
 *              binary name is set.
 * @return binary name
 */
const char *getConfigBinaryName(const struct ConfigParserState *state);
