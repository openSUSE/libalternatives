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

enum AlternativeLinkType
{
	ALTLINK_BINARY = 0,
	ALTLINK_MANPAGE = 1,

	ALTLINK_EOL = -1
};

struct AlternativeLink
{
	int priority;
	int type;
	const char *target;
};


// loads highest priority alternative
int loadDefaultAlternativesForBinary(const char *binary_name, struct AlternativeLink **alternatives);

int loadSpecificAlternativeForBinary(const char *binary_name, int prio, struct AlternativeLink **alternatives);

// int listAllAlternativesForBinary(const char *binary_name, int **alts);
// int loadAlternativeForBinary(int priority, const char *binary_name, struct AlternativeLink **alternatives);

// returns config override (priority) from a given config file
// 0 otherwise, or -1 on error
int loadConfigOverride(const char *binary_name, const char *config_path);

// convenience
void freeAlternatives(struct AlternativeLink **);

// convenience
int execDefault(char *argv[]); // binary in argv[0]
char* defaultManpage(const char *binary_name);
