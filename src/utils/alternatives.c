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
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "../libalternatives.h"

const char binname[] = "alts";

static int strcmpp(const void *a, const void *b, __attribute__((unused)) void *ptr)
{
	return strcmp(*(const char * const *)a, *(const char * const *)b);
}

static int numcmp(const void *a, const void *b, __attribute__((unused)) void *ptr)
{
	return *(const int*)a - *(const int*)b;
}

static int listAllInstalledOverrides(const char *program_filter)
{
	char **bins;
	size_t bin_size;

	if (listAllAvailableBinaries(&bins, &bin_size) != 0) {
		perror(binname);
		return -1;
	}

	qsort_r(&bins[0], bin_size, sizeof(char*), strcmpp, NULL);

	for (size_t i=0; i<bin_size; i++) {
		const char *binary = bins[i];
		int *priorities;
		size_t prio_size;
		const char *alt_no_str = "Alternatives: %d\n";

		if (program_filter != NULL && strcmp(program_filter, binary) != 0)
			continue;
		if (i > 0 && program_filter == NULL)
			puts("---");

		printf("Binary: %s\n", binary);
		if (listAllAlternativesForBinary(binary, &priorities, &prio_size) != 0) {
			if (errno == ENOENT) {
				printf(alt_no_str, 0);
				continue;
			}
			else {
				perror(binname);
				return -1;
			}
		}

		int override_src = 0;
		int def_priority = loadDefaultConfigOverride(binary, &override_src);

		printf(alt_no_str, prio_size);
		qsort_r(priorities, prio_size, sizeof(int), numcmp, NULL);

		if (prio_size > 0 && def_priority <= 0) {
			override_src = 0;
			def_priority = priorities[prio_size-1];
		}

		for (size_t i=0; i<prio_size; i++) {
			int priority = priorities[i];
			struct AlternativeLink *alts;
			char priority_mark = ' ';

			if (def_priority == priority) {
				switch (override_src) {
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

			if (loadSpecificAlternativeForBinary(binary, priority, &alts) < 0) {
				printf("*** %d priority data unparsable config.\n", priority);
				continue;
			}
			for (struct AlternativeLink *link=alts; link && link->type != ALTLINK_EOL; link++) {
				if (link->type == ALTLINK_BINARY) {
					printf("  Priority: %d%c  Target: %s\n", priority, priority_mark, link->target);
					break;
				}
			}
			freeAlternatives(&alts);
		}

		free(priorities);
	}
	free(bins);

	return 0;
}

static int setProgramOverride(const char *program, int priority, int is_system, int is_user)
{
	if (!is_user && !is_system) {
		if (geteuid() == 0)
			is_system = 1;
		else
			is_user = 1;
	}

	if (is_system && is_user) {
		fprintf(stderr, "Trying to override both system and user priorities. Decide on one.\n");
		return -1;
	}

	const char *config_fn = (is_user ? userConfigFile() : systemConfigFile());
	int ret = setConfigOverride(program, priority, config_fn);
	if (ret < 0) {
		perror(config_fn);
		fprintf(stderr, "Error updating override file\n");
	}

	return ret;
}

static void printHelp()
{
	puts(
		"\n\n"
		"  libalternatives (C) 2021  SUSE LLC\n"
		"\n"
		"    alts -h         --- this help screen\n"
		"    alts -l[name]   --- list programs or just one with given name\n"
		"    alts [-u] [-s] -n <program> [-p <alt_priority>]\n"
		"       sets an override with a given priority as default\n"
		"       if priority is not set, then resets to default by removing override\n"
		"       -u -- user override, default for non-root users\n"
		"       -s -- system overrude, default for root users\n"
		"       -n -- program to override with a given priority alternative\n"
		"\n\n"
	);
}

static void setFirstCommandOrError(int *command, int new_command)
{
	if (*command == 0)
		*command = new_command;
	else
		*command = -1;
}

static int processOptions(int argc, char *argv[])
{
	int opt;

	int command = 0;
	const char *program = NULL;
	int priority = 0;
	int is_system = 0, is_user = 0;

	optind = 1; // reset since we call this multiple times in unit tests
	while ((opt = getopt(argc, argv, ":hn:p:l::us")) != -1) {
		switch(opt) {
			case 'h':
			case 'r':
				setFirstCommandOrError(&command, opt);
				break;
			case 'u':
				is_user = 1;
				break;
			case 's':
				is_system = 1;
				break;
			case 'l':
			case 'n':
				setFirstCommandOrError(&command, opt);
				program = optarg;
				if (!optarg && optind < argc && argv[optind] != NULL && argv[optind][0] != '-') {
					program = argv[optind++];
				}
				break;
			case 'p': {
				char *ptr;
				errno = 0;
				priority = strtol(optarg, &ptr, 10);
				if (errno != 0 || *ptr != '\x00') {
					command = -1;
				}
				break;
			}
			default:
				fprintf(stderr, "unexpected return from getopt: %d\n", opt);
				setFirstCommandOrError(&command, opt);
		}
	}

	switch (command) {
		case 'h':
			printHelp();
			break;
		case '?':
		case -1:
		case 0:
			printHelp();
			return -1;
		case 'l':
			return listAllInstalledOverrides(program);
		case 'n':
			return setProgramOverride(program, priority, is_system, is_user);
		default:
			printf("unimplemented command %c %d\n", command, (int)command);
			return 10;
	}

	return 0;
}

#ifdef UNITTESTS
int alternatives_app_main(int argc, char *argv[])
{
#else
int main(int argc, char *argv[])
{
	if (strcmp(binname, basename(argv[0])) != 0)
		return execDefault(argv);
#endif
	return processOptions(argc, argv);
}
