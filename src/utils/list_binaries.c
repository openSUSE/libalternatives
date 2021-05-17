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

#define _GNU_SOURCE
#include "utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct InstalledBinaryData
{
	const char *binary_name;
	size_t num_priorities;
	int *priorities;

	int def_priority, def_priority_src;
	struct AlternativeLink **alts; // [0..num_priorities)
};

static int strcmpp(const void *a, const void *b, __attribute__((unused)) void *ptr)
{
	return strcmp(*(const char * const *)a, *(const char * const *)b);
}

static int numcmp(const void *a, const void *b, __attribute__((unused)) void *ptr)
{
	return *(const int*)a - *(const int*)b;
}

static void filterStringsArray(const char *filter, char **bins, size_t *bin_size)
{
	if (filter == NULL)
		return;

	size_t already_matched = 0;
	for (size_t i=0; i<*bin_size; i++) {
		char *binary_name = bins[i];

		if (already_matched == 0 && strcmp(binary_name, filter) == 0) {
			bins[0] = binary_name;
			already_matched = 1;
		}
		else {
			free(binary_name);
		}
	}

	*bin_size = already_matched;
}

static int loadInstalledBinariesAndTheirOverrides(const char *program_filter, struct InstalledBinaryData **binaries_ptr, size_t *bin_size)
{
	char **binary_names_array;

	if (listAllAvailableBinaries(&binary_names_array, bin_size) != 0) {
		perror(binname);
		return -1;
	}

	filterStringsArray(program_filter, binary_names_array, bin_size);
	qsort_r(&binary_names_array[0], *bin_size, sizeof(char*), strcmpp, NULL);
	*binaries_ptr = (struct InstalledBinaryData*)malloc(sizeof(struct InstalledBinaryData) * *bin_size);

	for (size_t i=0; i<*bin_size; i++) {
		struct InstalledBinaryData *binary = (*binaries_ptr) + i;
		binary->binary_name = binary_names_array[i];

		if (listAllAlternativesForBinary(binary->binary_name, &binary->priorities, &binary->num_priorities) != 0) {
			if (errno == ENOENT) {
				binary->num_priorities = 0;
				continue;
			}
			else {
				perror(binname);
				return -1;
			}
		}

		binary->def_priority_src = 0;
		binary->def_priority = loadDefaultConfigOverride(binary->binary_name, &binary->def_priority_src);


		qsort_r(binary->priorities, binary->num_priorities, sizeof(int), numcmp, NULL);

		if (binary->num_priorities > 0 && binary->def_priority <= 0) {
			binary->def_priority_src = 0;
			binary->def_priority = binary->priorities[binary->num_priorities-1];
		}

		binary->alts = (struct AlternativeLink**)malloc(sizeof(struct AlternativeLink*) * binary->num_priorities);

		for (size_t i=0; i<binary->num_priorities; i++) {
			int priority = binary->priorities[i];
			struct AlternativeLink *alts;

			if (loadSpecificAlternativeForBinary(binary->binary_name, priority, &alts) < 0) {
				alts = NULL;
			}

			binary->alts[i] = alts;
		}
	}
	free(binary_names_array);

	return 0;
}

static void printInstalledBinaryAlternatives(const struct InstalledBinaryData *binary)
{
	const char *alt_no_str = "Alternatives: %d\n";

	printf("Binary: %s\n", binary->binary_name);
	printf(alt_no_str, binary->num_priorities);

	for (size_t i = 0; i < binary->num_priorities; i++)
	{
		int priority = binary->priorities[i];
		struct AlternativeLink *alts = binary->alts[i];
		char priority_mark = ' ';

		if (binary->def_priority == priority)
		{
			switch (binary->def_priority_src)
			{
			default:
			case 0: // default selection
				priority_mark = '*';
				break;
			case 1: // system
				priority_mark = '!';
				break;
			case 2: // user
				priority_mark = '~';
				break;
			}
		}

		if (!alts) {
			printf("*** %d priority data unparsable config.\n", priority);
			continue;
		}
		for (struct AlternativeLink *link = binary->alts[i]; link && link->type != ALTLINK_EOL; link++)
		{
			if (link->type == ALTLINK_BINARY)
			{
				printf("  Priority: %d%c  Target: %s\n", priority, priority_mark, link->target);
				break;
			}
		}
	}
}

static void freeInstalledBinaryDataStruct(struct InstalledBinaryData *data)
{
	free((void*)data->binary_name);
	free(data->priorities);
	for (size_t i=0; i<data->num_priorities; i++)
		freeAlternatives(&data->alts[i]);
}

int printInstalledBinariesAndTheirOverrideStates(const char *program)
{
	struct InstalledBinaryData *binaries;
	size_t bin_size;
	int ret;

	ret = loadInstalledBinariesAndTheirOverrides(program, &binaries, &bin_size);
	if (ret != 0)
		return ret;

	for (size_t i=0; i<bin_size; i++) {
		if (i > 0)
			puts("---");
		printInstalledBinaryAlternatives(binaries + i);
		freeInstalledBinaryDataStruct(binaries + i);
	}
	free(binaries);

	return 0;
}
