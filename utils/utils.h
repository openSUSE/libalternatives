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

#include "../src/libalternatives.h"

extern const char binname[];

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

int printInstalledBinariesAndTheirSetting(const char *program);
int printInstalledBinariesAndTheirOverrideStates(const char *program);
int checkGroupConsistencies(const struct InstalledBinaryData *data, unsigned n_binaries, enum ConsistencyCheckFlags flags, struct ConsistencyError **errors, unsigned *n_errors);
