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
