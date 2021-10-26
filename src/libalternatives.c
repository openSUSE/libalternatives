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

//#define  _POSIX_C_SOURCE 202000L
#define __USE_MISC 1
#define _GNU_SOURCE 1

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "libalternatives.h"
#include "parser.h"

#if !defined(ETC_PATH)
#error "ETC_PATH is undefined"
#endif

#if !defined(CONFIG_FILENAME)
#error "CONFIG_FILENAME is undefined"
#endif

#define SYSTEM_OVERRIDE_PATH ETC_PATH "/" CONFIG_FILENAME

#if !defined(CONFIG_DIR)
#error "CONFIG_DIR is undefind"
#endif

const char system_override_path[] = "";
const char user_override_path[] = "";

#ifdef __GNUC__
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define PUBLIC_FUNC     __attribute__ ((visibility ("default")))
#else
#define unlikely(x)     (x)
#define PUBLIC_FUNC
#endif

int libalternatives_debug = 0;
#define IS_DEBUG unlikely(libalternatives_debug)

static char *__config_path = CONFIG_DIR;
static const char* getConfigDirectory()
{
	return __config_path;
}

#ifdef UNITTESTS
void setConfigDirectory(const char *config_directory)
{
	if (strcmp(__config_path, CONFIG_DIR) != 0)
		free(__config_path);
	__config_path = strdup(config_directory);
}
#endif

static void checkEnvDebug()
{
	const char *debug = secure_getenv("LIBALTERNATIVES_DEBUG");
	libalternatives_debug = ( debug != NULL && debug[0] == '1' && debug[1] == '\x00' );
}

static const char *concat_str_safe(const char *str1, int len1, const char *str2, int len2)
{
	char *str = malloc(len1 + len2);
	strncpy(str, str1, len1);
	strncpy(str+len1, str2, len2);
	return str;
}

// new_priority = priority from the filesystem
// old_priority = priority from old calls, or 0
// data = additional data
// return => 1 to set new priority, 0 to retain old
typedef int(*PriorityMatchFunction)(int new_priority, int old_priority, void *data);

static int PriorityMatch_highest(int a, int b, __attribute__((unused)) void *unused)
{
	return (a > b ? 1 : 0);
}

/*
static int PriorityMatch_highestButSmallerThanSupplied(int a, int b, int max)
{
	return (a < max && a > b ? 1 : 0);
}
*/

static int PriorityMatch_getExact(int a, __attribute__((unused)) int b, void *prio)
{
	return (a == *(int*)prio ? 1 : 0);
}

static int findAltConfig(const char *binary_name, PriorityMatchFunction priority_match_func, int *prio, void *data)
{
	int retfd = -1;
	int configdirfd = -1;
	int binaryconfigdirfd = -1;
	int saved_error = 0;
	DIR *dir = NULL;
	const char *filename = NULL;

	*prio = 0;

	configdirfd = open(getConfigDirectory(), O_DIRECTORY, O_RDONLY | O_CLOEXEC);
	if (configdirfd < 0)
		goto err;

	binaryconfigdirfd = openat(configdirfd, binary_name, O_DIRECTORY, O_RDONLY | O_CLOEXEC);
	if (binaryconfigdirfd < 0)
		goto err;

	close(configdirfd);
	configdirfd = -1;

	dir = fdopendir(dup(binaryconfigdirfd));
	if (dir == NULL)
		goto err;

	struct dirent *dirptr;

	errno = 0;
	while ((dirptr = readdir(dir)) != NULL) {
		if (dirptr->d_name[0] == '.')
			continue;

		switch (dirptr->d_type) {
			case DT_REG:
				break;
			case DT_UNKNOWN: {
				struct stat info;

				if (fstatat(binaryconfigdirfd, dirptr->d_name, &info, AT_SYMLINK_NOFOLLOW) < 0) {
					goto err;
				}

				if (S_ISREG(info.st_mode))
					break;
			}
			// fallthrough
			default:
				continue;
		}

		int new_prio = atoi(dirptr->d_name);
		if (priority_match_func(new_prio, *prio, data) == 1) {
			*prio = new_prio;
			free((void*)filename);
			filename = strdup(dirptr->d_name);
		}
	}

	if (errno == 0 && filename == NULL)
		errno = ENOENT;

	if (errno == 0)
		retfd = openat(binaryconfigdirfd, filename, 0, O_RDONLY | O_CLOEXEC);

err:
	saved_error = errno;

	if (configdirfd != -1)
		close(configdirfd);
	if (binaryconfigdirfd != -1)
		close(binaryconfigdirfd);
	if (dir != NULL)
		closedir(dir);

	free((void*)filename);
	errno = saved_error;

	return retfd;
}

static int loadAlternativeForBinary(const char *binary_name, PriorityMatchFunction matcher, int *prio, struct AlternativeLink **alternatives)
{
	struct OptionsParserState *state = NULL;
	int ret = -1;
	int fd = -1;

	*alternatives = NULL;

	{
		int data = *prio;
		fd = findAltConfig(binary_name, matcher, prio, &data);
		if (fd < 0)
			goto err;
	}

	state = initOptionsParser();

	struct stat stat_data;

	if (fstat(fd, &stat_data) < 0)
		goto err;

	if (stat_data.st_size > 10240) {
		fprintf(stderr, "options file with priority %d is unusually large. Truncating to 10kB", *prio);
		stat_data.st_size = 10240;
	}

	char buffer[1024];

	ret = 0;
	while (ret == 0 && stat_data.st_size > 0) {
		ssize_t s = read(fd, buffer, 1024);
		if (s > 0) {
			ret = parseOptionsData(buffer, s, state);
			stat_data.st_size -= s;
		}
		else if (s == 0) {
			fprintf(stderr, "options file with priority %d changed during reading?", *prio);
			break;
		}
		else {
			switch (errno) {
				case EINTR:
					continue;
				default:
					goto err;
			}
		}
	}

	if (stat_data.st_size != 0)
		ret = -1;

	if (ret == 0) {
		*alternatives = doneOptionsParser(*prio, state);
		state = NULL;
	}

err:
	if (state != NULL) {
		struct AlternativeLink *tmp = doneOptionsParser(*prio, state);
		if (tmp != NULL)
			libalts_free_alternatives_ptr(&tmp);
	}

	if (fd != -1)
		close(fd);

	return ret;
}

PUBLIC_FUNC
int libalts_load_highest_priority_binary_alternatives(const char *binary_name, struct AlternativeLink **alternatives)
{
	int prio = 0;
	return loadAlternativeForBinary(binary_name, PriorityMatch_highest, &prio, alternatives);
}

PUBLIC_FUNC
int libalts_load_exact_priority_binary_alternatives(const char *binary_name, int prio, struct AlternativeLink **alternatives)
{
	return loadAlternativeForBinary(binary_name, PriorityMatch_getExact, &prio, alternatives);
}

static int isDotPseudoDirectory(const char *name)
{
	const char null_byte = '\0';

	if (name[0] == '.') {
		switch (name[1]) {
			case '.':
				return name[2] == null_byte;
			default:
				return name[1] == null_byte;
		}
	}

	return 0;
}

PUBLIC_FUNC
int libalts_load_available_binaries(char ***binaries_ptr, size_t *size)
{
	errno = 0;
	*size = 0;
	*binaries_ptr = NULL;

	int saved_error = 0;
	int ret = -1;
	int fd = -1;
	DIR *dir = NULL;

	fd = open(getConfigDirectory(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if (fd == -1)
		goto err;

	dir = fdopendir(fd);
	if (dir == NULL) {
		close(fd);
		goto err;
	}

	size_t pos = 0;

	struct dirent *dirent;

	while ((dirent = readdir(dir)) != NULL) {
		if (pos >= *size) {
			*size = 2 * (*size + 1);
			*binaries_ptr = (char**)realloc(*binaries_ptr, sizeof(char*)**size);
		}
		if (*binaries_ptr == NULL)
			goto err;

		switch (dirent->d_type) {
			case DT_DIR:
				break;
			case DT_UNKNOWN: {
				// fall-back to detect directory via stat
				struct stat st;
				if (fstatat(fd, dirent->d_name, &st, 0) == -1)
					goto err;

				if (S_ISDIR(st.st_mode))
					break;
			}
			// fall-through
			default:
				// skip all non-directories
				continue;
		}
		if (isDotPseudoDirectory(dirent->d_name))
			continue;

		(*binaries_ptr)[pos++] = strdup(dirent->d_name);
	}

	*size = pos;
	ret = 0;

err:
	saved_error = errno;

	if (dir != NULL)
		closedir(dir);
	if (ret != 0 && *binaries_ptr != NULL) {
		size_t i;
		for(i = 0; i < pos; i++)
			free((*binaries_ptr)[i]);
		free(*binaries_ptr);
		*binaries_ptr = NULL;
	}

	if (ret != 0)
		errno = saved_error;

	return ret;
}

struct collectPrioData
{
	int **alts;
	size_t *size;
	size_t pos;
};

int collectAllPrioritiesInData(int new_prio, __attribute__((unused)) int old_prio, void *data_ptr)
{
	struct collectPrioData *data = (struct collectPrioData*)data_ptr;

	if (data->pos >= *data->size) {
		*data->size += 32;
		*data->alts = (int*)realloc(*data->alts, sizeof(int)**data->size);
	}

	(*data->alts)[data->pos++] = new_prio;
	return 1;
}

PUBLIC_FUNC
int libalts_load_binary_priorities(const char *binary_name, int **alts, size_t *size)
{
	int ignored;

	*size = 0;
	*alts = NULL;

	struct collectPrioData data = {alts, size, 0};
	int fd = findAltConfig(binary_name, collectAllPrioritiesInData, &ignored, &data);
	*size = data.pos;

	if (fd < 0)
		return fd;

	close(fd);
	return 0;
}

static ssize_t loadConfigData(const char *config_path, char *data, const ssize_t max_config_size)
{
	ssize_t ret = -1;
	int fd = -1;

	fd = open(config_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (fd < 0)
		goto end;

	struct stat stat_data;
	if (fstat(fd, &stat_data) < 0) {
		switch (errno) {
			case ENOENT:
				ret = 0;
				goto end;
			default:
				goto end;
		}
	}

	if (stat_data.st_size >= max_config_size) {
		fprintf(stderr, "ignoring libalternatives config file: %s. Too large.\n", config_path);
		ret = 0;
		goto end;
	}

	ret = 0;
	while (stat_data.st_size != 0) {
		ssize_t rs = read(fd, data + ret, stat_data.st_size);
		if (rs == 0 && stat_data.st_size > 0) {
			fprintf(stderr, "libalternatives config seems to have changed: %s. Trying to parse anyway\n", config_path);
			break;
		}
		else if (rs < 0) {
			switch (errno) {
				case EINTR:
					continue;
				default:
					ret = -1;
					goto end;
			}
		}
		else {
			stat_data.st_size -= rs;
			ret += rs;
		}
	}
	data[ret] = '\x00';

end:
	if (fd != -1)
		close(fd);
	if (ret <= 0)
		data[0] = '\x00';
	return ret;
}

PUBLIC_FUNC
int libalts_read_binary_configured_priority_from_file(const char *binary_name, const char *config_path)
{
	const ssize_t max_config_size = 1 << 10;
	char data[max_config_size];
	data[0] = '\0';
	loadConfigData(config_path, data, max_config_size);

	struct ConfigParserState *state = initConfigParser(binary_name);
	int prio = parseConfigData(data, state);
	doneConfigParser(state);
	return prio;
}

static int saveConfigData(const char *config_path, const char *data)
{
	const char *saved_path = concat_str_safe(config_path, strlen(config_path), ".new", 5);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	struct stat st;
	const size_t len = strlen(data);
	int ret = 0;
	int olderr;
	size_t pos;

	if (stat(config_path, &st) == 0) {
		mode = st.st_mode & (S_IRWXO | S_IRWXG | S_IRWXU);
	}
	int fd = open(saved_path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, mode);
	if (fd < 0) {
		ret = -1;
		goto ret;
	}

	pos = 0;
	while (pos < len) {
		int write_count = write(fd, data+pos, len-pos);
		if (write_count > 0) {
			pos += write_count;
		}
		else {
			switch (errno) {
				case EINTR:
					break;
				default:
					ret = -1;
					goto ret;
			}
		}
	}

	if (close(fd) == -1 && errno != EINTR) {
		fd = -1;
		ret = -1;
		goto ret;
	}

	fd = -1;

	ret = rename(saved_path, config_path);

ret:
	olderr = errno;
	free((void*)saved_path);
	errno = olderr;
	if (fd != -1)
		close(fd);
	return ret;
}

PUBLIC_FUNC
int libalts_write_binary_configured_priority_to_file(const char *binary_name, int priority, const char *config_path)
{
	const ssize_t max_config_size = 1 << 10;
	char data[max_config_size];
	data[0] = '\0';
	loadConfigData(config_path, data, max_config_size);

	struct ConfigParserState *state = initConfigParser(binary_name);
	parseConfigData(data, state);
	const char *new_data;
	if (priority == 0)
		new_data = resetToDefaultPriorityAndReturnUpdatedConfig(state);
	else
		new_data = setBinaryPriorityAndReturnUpdatedConfig(priority, state);

	int ret = -1;

	if (new_data != NULL)
		ret = saveConfigData(config_path, new_data);

	doneConfigParser(state);
	return ret;
}

PUBLIC_FUNC
void libalts_free_alternatives_ptr(struct AlternativeLink **links)
{
	for (struct AlternativeLink *ptr=*links; ptr != NULL && ptr->type != ALTLINK_EOL; ptr++)
		free((void*)ptr->target);

	free(*links);
	*links = NULL;
}

PUBLIC_FUNC
int libalts_read_configured_priority(const char *binary_name, int *src)
{
	// try to load user override
	const char *config_path = libalts_get_user_config_path();
	int priority = 0;
	if (config_path != NULL) {
		if (IS_DEBUG)
			fprintf(stderr, "Trying to load user override for %s from: %s\n", binary_name, config_path);
		priority = libalts_read_binary_configured_priority_from_file(binary_name, config_path);
		if (IS_DEBUG)
			fprintf(stderr, "user override priority: %d\n", priority);
		if (unlikely(src != NULL)) {
			if (priority > 0) {
				*src = 2;
			}
		}
	}

	// if not loaded, try system override
	if (priority <= 0) {
		priority = libalts_read_binary_configured_priority_from_file(binary_name, SYSTEM_OVERRIDE_PATH);
		if (IS_DEBUG)
			fprintf(stderr, "system override priority: %d\n", priority);
		if (unlikely(src != NULL)) {
			if (priority > 0) {
				*src = 1;
			}
		}
	}

	return priority;
}

PUBLIC_FUNC
const char* libalts_get_system_config_path()
{
	return SYSTEM_OVERRIDE_PATH;
}

static const char* __override_path;
PUBLIC_FUNC
const char* libalts_get_user_config_path()
{
	if (__override_path == NULL) {
		const char *config_home = secure_getenv("XDG_CONFIG_HOME");
		if (config_home != NULL) {
			const char config_filename[] = "/" CONFIG_FILENAME;
			__override_path = concat_str_safe(config_home, strlen(config_home), config_filename, sizeof(config_filename));
		}
		else {
			config_home = secure_getenv("HOME");
			const char config_filename[] = "/.config/" CONFIG_FILENAME;
			if (config_home != NULL)
				__override_path = concat_str_safe(config_home, strlen(config_home), config_filename, sizeof(config_filename));
		}
	}

	return __override_path;
}

// for debugging
void setConfigPath(const char *config_path)
{
	if (__override_path)
		free((void*)__override_path);

	__override_path = NULL;
	if (config_path)
		__override_path = strdup(config_path);
}

static int loadAlternatives(const char *binary_name, struct AlternativeLink **alts)
{
	int priority = libalts_read_configured_priority(binary_name, NULL);

	int ret = 0;
	if (priority > 0) {
		ret = libalts_load_exact_priority_binary_alternatives(binary_name, priority, alts);
		if (unlikely(ret != 0)) {
			if (IS_DEBUG)
				fprintf(stderr, "failed to load override priority %d - reseting to default", priority);
			priority = 0;
		}
	}
	if (priority == 0)
		ret = libalts_load_highest_priority_binary_alternatives(binary_name, alts);

	if (IS_DEBUG)
		fprintf(stderr, "loaded alternatives?: %d\n", ret);

	return ret;
}

PUBLIC_FUNC
int libalts_exec_default(char *argv[])
{
	argv[0]=basename(argv[0]);

	struct AlternativeLink *alts;
	checkEnvDebug();
	loadAlternatives(argv[0], &alts);

	if (alts) {
		while (alts->type != ALTLINK_EOL) {
			if (alts->type == ALTLINK_BINARY) {
				if ((alts->options & ALTLINK_OPTIONS_KEEPARGV0) == 0)
					argv[0] = (char*)alts->target;
				execv(alts->target, argv);
				perror("Failed to execute target.");
				break;
			}
			alts++;
		}
	}

	if (IS_DEBUG)
		fprintf(stderr, "execDefault() failed with target %s\n", (alts ? alts->target : NULL));
	errno = ENOENT;
	return -1;
}

PUBLIC_FUNC
char** libalts_get_default_manpages(const char *binary_name)
{
	struct AlternativeLink *alts;
	checkEnvDebug();
	loadAlternatives(binary_name, &alts);

	size_t size = 16, pos = 0;
	char **manpages = malloc(sizeof(char*)*size);

	if (alts) {
		struct AlternativeLink *ptr = alts;
		while (ptr->type != ALTLINK_EOL && pos < size-1) {
			if (ptr->type == ALTLINK_MANPAGE)
				manpages[pos++] = strdup(ptr->target);
			ptr++;
		}

		libalts_free_alternatives_ptr(&alts);
	}

	manpages[pos] = NULL;
	return manpages;
}
