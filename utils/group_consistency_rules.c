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

#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "utils.h"

static unsigned numBinariesInGroup(struct AlternativeLink *data)
{
	unsigned size = 0;

	for (; data->type != ALTLINK_EOL; data++)
		if (data->type == ALTLINK_GROUP)
			size++;

	return size;
}

static int isBinaryInGroupOfBinariesOrWithoutGroup(const char *binary_name, struct AlternativeLink *data)
{
	int ret = 1;

	for (; data->type != ALTLINK_EOL; data++) {
		if (data->type == ALTLINK_GROUP) {
			ret = 0;

			if (strcmp(binary_name, data->target) == 0)
				return 1;
		}
	}
	return ret;
}

static int isLargestOrSameSizeGroupAsLowerPriorityBinary(unsigned *group_sizes, unsigned group_idx)
{
	if (group_idx == 0 || group_sizes[group_idx-1] <= group_sizes[group_idx])
		return 1;
	return 0;
}

static void appendError(const struct AlternativeLink *data, const char *message, struct ConsistencyError **errors_ptr, unsigned *n_err)
{
	if (errors_ptr == NULL)
		return;

	struct ConsistencyError *errors = *errors_ptr;
	unsigned idx = *n_err;

	errors = *errors_ptr = (struct ConsistencyError*)realloc(errors, sizeof(struct ConsistencyError) * (idx + 1));
	errors[idx].message = message;
	errors[idx].record = data;

	*n_err = idx + 1;
}

int checkGroupConsistencies(const struct InstalledBinaryData *data, unsigned n_binaries, enum ConsistencyCheckFlags flags, struct ConsistencyError **errors, unsigned *n_errors)
{
	(void)flags;
	int ret = 0;

	if (n_errors != NULL)
		*n_errors = 0;

	for (unsigned i=0; i<n_binaries; i++) {
		const struct InstalledBinaryData *d = data + i;
		unsigned *group_sizes = (unsigned*)malloc(sizeof(unsigned) * d->num_priorities);

		for (unsigned pidx=0; pidx<d->num_priorities; pidx++) {
			struct AlternativeLink *alts = d->alts[pidx];

			if (alts == NULL) {
				ret |= 1;
				continue;
			}

			if (!isBinaryInGroupOfBinariesOrWithoutGroup(d->binary_name, alts)) {
				appendError(alts, "WARNING: binary not part of the Group", errors, n_errors);
				ret |= 1;
			}

			group_sizes[pidx] = numBinariesInGroup(d->alts[pidx]);
			if (!isLargestOrSameSizeGroupAsLowerPriorityBinary(group_sizes, pidx)) {
				appendError(alts, "WARNING: shadows more complete Group with lower priority", errors, n_errors);
				ret |= 1;
			}
		}

		free(group_sizes);
	}
	return ret;
}
