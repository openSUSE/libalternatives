#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "parser.h"

/*----------------------------------------------------------------------*/

struct ConfigParserState
{
  char *binary_name;
  u_int32_t priority;
  u_int32_t line_number;   /* file line nr. */
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

/*----------------------------------------------------------------------*/

struct ConfigParserState* initConfigParser(const char *binary_name)
{
  struct ConfigParserState *state = malloc(sizeof(struct ConfigParserState));

  state->binary_name = strdup(binary_name);
  state->priority = 0;
  state->line_number = -1;

  return state;  
}

int parseConfigData(const char *buffer,
		    struct ConfigParserState *state)
{
  const char *line;
  int line_number = 0;
  
  line = strsep(&buffer, "\n");
  if (line == NULL)
    return 0;

  do {
    line_number++;
    const char *equal_pos = strstr(line,"=");
    if (equal_pos != NULL)
    {
      /* evaluating key (binary_name) */
      char *raw_key = NULL;      
      raw_key = strndup(line,equal_pos-line);
      char key[strlen(raw_key)];
      strcpy(key,trim(raw_key)); /* strip whitespaces */
      free(raw_key);
      if (strcmp(key,state->binary_name) != 0)
	continue; /* not the binary key */
    } else {
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
	|| (*trim(endptr) != '\0') /* There is still a rest */) {
      free(raw_value);
      continue;
    }
    free(raw_value);    
    
    state->priority = (int) val;
    state->line_number = line_number;
    return (int) val;
  } while ((line = strsep(&buffer, "\n")) != NULL);

  return 0;
}

void doneConfigParser(struct ConfigParserState *state)
{
  if (state != NULL) {
    free(state->binary_name);
    free(state);
  }
}

int getConfigLineNr(const struct ConfigParserState *state)
{
  if (state == NULL)
    return -1;
  return state->line_number;
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

void setConfigPriority(int priority,
			     struct ConfigParserState *state)
{
  if (state != NULL) {
    state->priority = priority;    
  }
}
void resetConfigDefaultPriority(struct ConfigParserState *state)
{
  if (state != NULL) {
    state->priority = 0;
    state->line_number = -1;
  }  
}
