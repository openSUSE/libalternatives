/*  libalternatives - update-alternatives alternative
 *  Copyright (C) 2021  SUSE LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <memory.h>

#include "libalternatives.h"
#include "parser.h"

#ifndef MIN
#define MIN(a,b) ((a<b)?(a):(b))
#endif

typedef int(*ParserFunction)(const char *data, size_t len, struct OptionsParserState *state);

struct OptionsParserState
{
	u_int32_t parser_func_param, parser_func_param2, parser_func_param3;
	ParserFunction parser_func;
	u_int32_t options;

	struct AlternativeLink *parsed_data;
	int parsed_data_size;
};

static int isWhitespace(const char c)
{
	return c == ' ' || c == '\t';
}

static int isDelimeter(const char c)
{
	return c == '=';
}

static int parser_alreadyErrorNoResumePossible(const char *data, size_t len, struct OptionsParserState *state)
{
	(void)data;
	(void)len;
	(void)state;

	return -1;
}

static void allocateBuffer(struct AlternativeLink **link, int *size_ptr)
{
	int size = *size_ptr;
	if (*link == NULL || size < 8)
		size += 8;

	*link = realloc(*link, sizeof(struct AlternativeLink) * size);

	for (int i=*size_ptr; i<size; ++i) {
		(*link+i)->type = ALTLINK_EOL;
		(*link+i)->target = 0;
	}
	*size_ptr = size;
}

static int findFirstParsedDataLocation(struct OptionsParserState *state, int type)
{
	for (int i=0; i<state->parsed_data_size - 1; ++i) {
		struct AlternativeLink *data = state->parsed_data + i;
		if (data->type == type)
			return i;

		if (data->type == ALTLINK_EOL) {
			data->type = type;
			return i;
		}
	}

	allocateBuffer(&state->parsed_data, &state->parsed_data_size);
	return findFirstParsedDataLocation(state, type);
}

static int findFirstFreeDataLocation(struct OptionsParserState *state, int type)
{
	int pos = findFirstParsedDataLocation(state, ALTLINK_EOL);
	(state->parsed_data+pos)->type = type;
	return pos;
}

static int parser_parseValue(const char *data, size_t len, struct OptionsParserState *state);
static int parser_skipOptionalWhitespaceBeforeManpageEntries(const char *data, size_t len, struct OptionsParserState *state)
{
	while (len>0) {
		if (!isWhitespace(*data)) {
			state->parser_func = parser_parseValue;
			return state->parser_func(data, len, state);
		}

		len--;
		data++;
	}

	return 0;
}

static int parser_searchToken(const char *data, size_t len, struct OptionsParserState *state);
static int parser_parseValue(const char *data, size_t len, struct OptionsParserState *state)
{
	struct AlternativeLink *link = state->parsed_data + state->parser_func_param;
	u_int16_t value_string_pos = state->parser_func_param2 & 0xFFFF;
	u_int16_t value_string_size = state->parser_func_param2 >> 16;
	const int is_multi_value = state->parser_func_param3 & 0x1;

	while (len > 0) {
		char c = *data;
		char *out = (char*)link->target;

		if (value_string_pos >= value_string_size) {
			value_string_size += 0x100;
			link->target = realloc((void*)link->target, value_string_size);
			out = (char*)link->target;
		}

		switch (c) {
			case ',':
				if (is_multi_value) {
					out[value_string_pos] = '\x00';
					while (value_string_pos > 0 && isWhitespace(out[value_string_pos-1])) {
						out[--value_string_pos] = '\x00';
					}

					if (value_string_pos > 0) {
						state->parser_func_param = findFirstFreeDataLocation(state, link->type);
						state->parser_func_param2 = 0;
					}

					state->parser_func = parser_skipOptionalWhitespaceBeforeManpageEntries;
					return state->parser_func(data+1, len-1, state);
				}
				out[value_string_pos] = c;
				break;
			case '\n':
			case '\r':
			case '\0':
				out[value_string_pos] = '\x00';
				while (value_string_pos > 0 && isWhitespace(out[value_string_pos-1])) {
					out[--value_string_pos] = '\x00';
				}

				state->parser_func = parser_searchToken;
				return state->parser_func(data+1, len-1, state);
			default:
				out[value_string_pos] = c;
				break;
		}

		data++;
		len--;
		value_string_pos++;
	}

	state->parser_func_param2 = value_string_pos | (((u_int32_t)value_string_size) << 16);
	return 0;
}

static int parser_parseMultiValue(const char *data, size_t len, struct OptionsParserState *state)
{
	state->parser_func_param3 = 1;
	state->parser_func = parser_parseValue;

	return state->parser_func(data, len, state);
}

struct valid_option_string {
	const char *label;
	int options;
};

static int parser_parseOptions(const char *data, size_t len, struct OptionsParserState *state);

static int parser_skipWhitespaceBeforeOption(const char *data, size_t len, struct OptionsParserState *state)
{
	while (len > 0 && isWhitespace(*data)) {
		len--;
		data++;
	}

	if (len == 0)
		return 0;

	state->parser_func = parser_parseOptions;
	return state->parser_func(data, len, state);
}

static int parser_skipWhitespaceBeforeCommaAndOption(const char *data, size_t len, struct OptionsParserState *state)
{
	while (len > 0 && isWhitespace(*data)) {
		len--;
		data++;
	}

	if (len == 0)
		return 0;

	if (*data != ',' && *data != '\n')
		state->parser_func = parser_alreadyErrorNoResumePossible;

	if (*data == ',') {
		len--;
		data++;
	}

	state->parser_func = parser_skipWhitespaceBeforeOption;
	return state->parser_func(data, len, state);
}

static int parser_asssertOption_KeepArgv0(const char *data, size_t len, struct OptionsParserState *state)
{
	if (len == 0)
		return 0;

	const char token[] = "KeepArgv0";
	u_int32_t pos = state->parser_func_param;
	u_int32_t max_pos_to_match = MIN(sizeof(token)-1, pos+len);

	while (pos < max_pos_to_match && *data == token[pos]) {
		pos++;
		data++;
		len--;
	}

	if (pos == max_pos_to_match) {
		if (pos == sizeof(token)-1) {
			state->parser_func = parser_skipWhitespaceBeforeCommaAndOption;
			state->options |= ALTLINK_OPTIONS_KEEPARGV0;
		}
		else
			state->parser_func_param = pos;
	}
	else
		state->parser_func = parser_alreadyErrorNoResumePossible;

	return state->parser_func(data, len, state);
}

static int parser_parseOptions(const char *data, size_t len, struct OptionsParserState *state)
{
	if (len == 0)
		return 0;

	switch (*data) {
		case 'K':
			state->parser_func = parser_asssertOption_KeepArgv0;
			state->parser_func_param = 1;
			break;
		case '\n':
		case '\r':
			state->parser_func = parser_searchToken;
			break;
		default:
			state->parser_func = parser_alreadyErrorNoResumePossible;
			break;
	}

	return state->parser_func(data+1, len-1, state);
}

#define AFTER_OFFSET 1000
enum WhiteSpaceSkipType
{
	BINARY_FUNCTION_BEFORE_EQUAL=1,
	BINARY_FUNCTION_AFTER_EQUAL=BINARY_FUNCTION_BEFORE_EQUAL+AFTER_OFFSET,
	MAN_FUNCTION_BEFORE_EQUAL=2,
	MAN_FUNCTION_AFTER_EQUAL=MAN_FUNCTION_BEFORE_EQUAL+AFTER_OFFSET,
	GROUP_FUNCTION_BEFORE_EQUAL=3,
	GROUP_FUNCTION_AFTER_EQUAL=GROUP_FUNCTION_BEFORE_EQUAL+AFTER_OFFSET,
	OPTIONS_FUNCTION_BEFORE_EQUAL=4,
	OPTIONS_FUNCTION_AFTER_EQUAL=OPTIONS_FUNCTION_BEFORE_EQUAL+AFTER_OFFSET,
};

static int parser_skipOptionalWhiteSpace(const char *data, size_t len, struct OptionsParserState *state)
{
	while (len > 0 && isWhitespace(*data)) {
		len--;
		data++;
	}

	if (len == 0)
		return 0;

	switch (state->parser_func_param) {
		case BINARY_FUNCTION_BEFORE_EQUAL:
		case MAN_FUNCTION_BEFORE_EQUAL:
		case GROUP_FUNCTION_BEFORE_EQUAL:
		case OPTIONS_FUNCTION_BEFORE_EQUAL:
			if (!isDelimeter(*data)) {
				state->parser_func = parser_alreadyErrorNoResumePossible;
				break;
			}

			data++;
			len--;

			state->parser_func_param = state->parser_func_param + AFTER_OFFSET;
			break;

		case BINARY_FUNCTION_AFTER_EQUAL:
			state->parser_func = parser_parseValue;

			state->parser_func_param = findFirstParsedDataLocation(state, ALTLINK_BINARY);
			state->parser_func_param2 = 0;
			state->parser_func_param3 = 0;
			break;
		case MAN_FUNCTION_AFTER_EQUAL:
			state->parser_func = parser_parseMultiValue;

			state->parser_func_param = findFirstParsedDataLocation(state, ALTLINK_MANPAGE);
			state->parser_func_param2 = 0;
			break;
		case GROUP_FUNCTION_AFTER_EQUAL:
			state->parser_func = parser_parseMultiValue;

			state->parser_func_param = findFirstParsedDataLocation(state, ALTLINK_GROUP);
			state->parser_func_param2 = 0;
			state->parser_func_param3 = 0;
			break;
		case OPTIONS_FUNCTION_AFTER_EQUAL:
			state->parser_func = parser_skipWhitespaceBeforeOption;
			break;
	}

	return state->parser_func(data, len, state);
}

static int assertTokenMatch(const char *data, size_t len, struct OptionsParserState *state, const char *match, int match_len, int whiteSpace_param)
{
	int pos = state->parser_func_param;
	int end_pos = match_len - pos + 1 <= (int)len ? match_len - pos + 1 : (int)len + pos;

	while (pos < end_pos && *data == match[pos]) {
		data++;
		pos++;
		len--;
	}

	if (pos != end_pos) {
		state->parser_func = parser_alreadyErrorNoResumePossible;
		return -1;
	}

	if (len == 0 && pos < match_len) {
		state->parser_func_param = pos;
		return 0;
	}
	state->parser_func = parser_skipOptionalWhiteSpace;
	state->parser_func_param = whiteSpace_param;
	return state->parser_func(data, len, state);
}

static int parser_assertBinaryToken(const char *data, size_t len, struct OptionsParserState *state)
{
	const char match[] = "binary";
	return assertTokenMatch(data, len, state, match, sizeof(match)-1, BINARY_FUNCTION_BEFORE_EQUAL);
}

static int parser_assertManpageToken(const char *data, size_t len, struct OptionsParserState *state)
{
	const char match[] = "man";

	return assertTokenMatch(data, len, state, match, sizeof(match)-1, MAN_FUNCTION_BEFORE_EQUAL);
}

static int parser_assertGroupToken(const char *data, size_t len, struct OptionsParserState *state)
{
	const char match[] = "group";

	return assertTokenMatch(data, len, state, match, sizeof(match)-1, GROUP_FUNCTION_BEFORE_EQUAL);
}

static int parser_assertOptionsToken(const char *data, size_t len, struct OptionsParserState *state)
{
	const char match[] = "options";

	return assertTokenMatch(data, len, state, match, sizeof(match)-1, OPTIONS_FUNCTION_BEFORE_EQUAL);
}

static int parser_searchToken(const char *data, size_t len, struct OptionsParserState *state)
{
	while (len > 0 && (isWhitespace(*data) || *data == '\n' || *data == '\r')) {
		data++;
		len--;
	}

	if (len == 0)
		return 0;

	switch (data[0]) {
		case 'b':
			state->parser_func_param = 1;
			state->parser_func = parser_assertBinaryToken;
			break;
		case 'g':
			state->parser_func_param = 1;
			state->parser_func = parser_assertGroupToken;
			break;
		case 'm':
			state->parser_func_param = 1;
			state->parser_func = parser_assertManpageToken;
			break;
		case 'o':
			state->parser_func_param = 1;
			state->parser_func = parser_assertOptionsToken;
			break;
		default:
			state->parser_func = parser_alreadyErrorNoResumePossible;
	};

	return state->parser_func(data+1, len-1, state);
}

struct OptionsParserState* initOptionsParser()
{
	struct OptionsParserState *state = malloc(sizeof(struct OptionsParserState));

	state->parser_func = parser_searchToken;
	state->parser_func_param = 0;
	state->parsed_data = NULL;
	state->parsed_data_size = 0;

	state->options = 0;

	return state;
}

int parseOptionsData(const char *buffer, size_t len, struct OptionsParserState *state)
{
	return state->parser_func(buffer, len, state);
}

struct AlternativeLink* doneOptionsParser(int priority, struct OptionsParserState *state)
{
	int errors = 0;
	while (state->parser_func != parser_searchToken && (errors = state->parser_func("\n", 1, state)) == 0);

	if (errors != 0) {
		free(state->parsed_data);
		free(state);
		return NULL;
	}

	struct AlternativeLink *parsed_data = state->parsed_data;
	for (struct AlternativeLink *ptr = parsed_data; ptr && ptr->type != ALTLINK_EOL; ptr++) {
		ptr->priority = priority;
		ptr->options = state->options;
	}

	free(state);
	return parsed_data;
}
