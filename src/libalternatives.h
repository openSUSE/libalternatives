/*  libalternatives - update-alternatives alternative
 *  Copyright © 2021  SUSE LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum AlternativeLinkType
{
	ALTLINK_BINARY = 0,
	ALTLINK_MANPAGE = 1,
	ALTLINK_GROUP = 2,

	ALTLINK_EOL = -1
};

enum AlternativeLinkOptions
{
	ALTLINK_OPTIONS_NONE = 0,
	ALTLINK_OPTIONS_KEEPARGV0 = 1,
};

struct AlternativeLink
{
	int priority;
	int type;
	const char *target;
	int options;
};


// loads highest priority alternative
int libalts_load_highest_priority_binary_alternatives(const char *binary_name, struct AlternativeLink **alternatives);

int libalts_load_exact_priority_binary_alternatives(const char *binary_name, int prio, struct AlternativeLink **alternatives);

int libalts_load_available_binaries(char ***binaries, size_t *size);
int libalts_load_binary_priorities(const char *binary_name, int **alts, size_t *size);

// returns config override (priority) from a given config file
// 0 otherwise, or -1 on error
int libalts_read_binary_configured_priority_from_file(const char *binary_name, const char *config_path);

// return 0 on success and -1 on error
int libalts_write_binary_configured_priority_to_file(const char *binary_name, int priority, const char *config_path);

// returns config override (priority) from default config files
// set src => 1 for system, 2 for user
// returns 0 if not overriden or -1 on error
int libalts_read_configured_priority(const char *binary_name, int *src);

// config filenames, they may or may not exist
const char* libalts_get_system_config_path();
const char* libalts_get_user_config_path();

// convenience
void libalts_free_alternatives_ptr(struct AlternativeLink **);

// convenience
int libalts_exec_default(char *argv[]); // binary in argv[0]

// returns a list of manpages followed by a NULL ptr
// returned data should be freed
char** libalts_get_default_manpages(const char *binary_name);

// for unit testing only, remove from library symbols later
#ifdef UNITTESTS
void setConfigDirectory(const char *config_directory);
#endif

#ifdef __cplusplus
}
#endif
