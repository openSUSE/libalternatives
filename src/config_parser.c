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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#include "parser.h"

/*----------------------------------------------------------------------*/

struct ConfigParserState
{
  char *binary_name;
  u_int32_t priority;
  /* The complete buffer which has been initialiazed by the call
   * parseConfigData and can be changed by the set/reset calls.
   */
  char *complete_content;
};

/*------------------------------static----------------------------------*/

static char *ltrim(char *s)
{
    while(isspace(*s)) s++;
    return s;
}

static char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

static char *trim(char *s)
{
    return rtrim(ltrim(s));
}

static bool is_key(const char *buffer, const char *key, int buffer_len)
{
  int ret = true;
  char *raw_key, *parsed_key = NULL;
  raw_key = strndup(buffer,buffer_len);
  parsed_key = strdup(trim(raw_key)); /* strip whitespaces */
  free(raw_key);
  if (strcmp(parsed_key,key) != 0)
    ret = false;
  free(parsed_key);
  return ret;
}

static void free_buffer(char **buffer)
{
  free(*buffer);
}

/** @brief Set priority for a binary name in the given state struct.
 *
 * @param priority Priority which has to be set. The entry will be deleted
                   if priority is <= 0.
 * @param state Reference on an ConfigParserState object in which the
 *              priority will be set.
 * @return Complete string buffer (with other values)
 */
static const char *setBinaryPriority(int priority, struct ConfigParserState *state)
{
  char *buf = state->complete_content;
  state->complete_content = strdup("");
  bool entry_found = false;
  char *line;
  while ((line = strsep(&buf, "\n")) != NULL) {
    const char *equal_pos = strstr(line,"=");
    if (equal_pos != NULL)
    {
      /* check key (binary_name) */
      if (is_key(line, state->binary_name, equal_pos-line))
      {
	if (!entry_found)
	{
	  entry_found = true;
	  if (priority>0)
	  {
	    /* update; entry will be removed if priority <= 0 */
	    char *content = state->complete_content;
	    asprintf(&state->complete_content, "%s%s%s=%d", content,
		     strlen(content) == 0 ? "" : "\n",
		     state->binary_name, priority);
	    free(content);
	  }
	}
	continue;
      }
    }
    char *content = state->complete_content;
    asprintf(&state->complete_content, "%s%s%s", content,
	     strlen(content) == 0 ? "" : "\n",
	     line);
    free(content);
  }
  free(buf);

  if (!entry_found && priority > 0)
  {
    /* appending */
    char *content = state->complete_content;
    asprintf(&state->complete_content, "%s%s%s=%d", content,
	     strlen(content) == 0 ? "" : "\n",
	     state->binary_name, priority);
    free(content);
  }

  return state->complete_content;
}

/*----------------------------------------------------------------------*/

struct ConfigParserState* initConfigParser(const char *binary_name)
{
  if (binary_name == NULL)
    return NULL;

  struct ConfigParserState *state = (struct ConfigParserState*)malloc(sizeof(struct ConfigParserState));

  state->binary_name = strdup(binary_name);
  state->priority = 0;
  state->complete_content = strdup("");

  return state;
}

int parseConfigData(const char *buffer, struct ConfigParserState *state)
{
  /* strsep changes the buffer. So we need a copy of it.*/
  char *begin_buf __attribute__ ((__cleanup__(free_buffer))) = strdup(buffer);
  char *buf = begin_buf;

  state->complete_content = strdup(buffer);

  char *line;
  while (state->priority == 0 && (line = strsep(&buf, "\n")) != NULL) {
    const char *equal_pos = strstr(line,"=");
    if (equal_pos != NULL)
    {
      /* check key (binary_name) */
      if (!is_key(line, state->binary_name, equal_pos-line))
      {
        continue; /* not the binary key */
      }
    } else {
      /* no "=" found */
      continue;
    }

    /* evaluating priority */
    const char *comment_pos = strstr(line,"#"); /* stripping comment */
    char *raw_value = NULL;
    if (comment_pos == NULL) {
      raw_value = strdup(equal_pos+1);
    } else {
      raw_value = strndup(equal_pos+1,comment_pos-equal_pos-1);
    }
    char *endptr = NULL;
    long int val = strtol(raw_value, &endptr, 10);
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
        || (errno != 0 && val == 0)
        || (endptr == raw_value)   /* No digits were found. */
        || (*trim(endptr) != '\0') /* There is still a rest */
        || val <= 0) {
      free(raw_value);
      continue;
    }
    free(raw_value);

    state->priority = (int) val;
  }

  return state->priority;
}

void doneConfigParser(struct ConfigParserState *state)
{
  if (state != NULL) {
    free(state->binary_name);
    free(state->complete_content);
    free(state);
  }
}

int getConfigPriority(const struct ConfigParserState *state)
{
  if (state == NULL)
    return -1;
  return state->priority;
}

const char *getConfigBinaryName(const struct ConfigParserState *state)
{
  if (state == NULL)
    return NULL;
  return state->binary_name;
}

const char *setBinaryPriorityAndReturnUpdatedConfig(int priority, struct ConfigParserState *state)
{
  if (state == NULL || priority <= 0)
    return NULL;

  state->priority = priority;
  return setBinaryPriority(priority, state);
}

const char *resetToDefaultPriorityAndReturnUpdatedConfig(struct ConfigParserState *state)
{
  if (state == NULL)
    return NULL;

  state->priority = 0;
  return setBinaryPriority(0, state);
}
