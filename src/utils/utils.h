/*  libalternatives - update-alternatives alternative
 *  Copyright (C) 2021  SUSE LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
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

#include "../libalternatives.h"

extern const char *binname;

struct ConsistencyError {
	const struct AlternativeLink *record;
	const char *message;
};

struct InstalledBinaryData
{
	const char *binary_name;
	size_t num_priorities;
	int *priorities;

	int def_priority, def_priority_src;
	struct AlternativeLink **alts; // [0..num_priorities)
};

enum ConsistencyCheckFlags
{
	CONSISTENCY_LOAD_ADDITIONAL_BINARIES = 1
};

extern int printInstalledBinariesAndTheirOverrideStates(const char *program);
extern int checkGroupConsistencies(const struct InstalledBinaryData *data, unsigned n_binaries, enum ConsistencyCheckFlags flags, struct ConsistencyError **errors, unsigned *n_errors);
