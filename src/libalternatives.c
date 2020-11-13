//#define  _POSIX_C_SOURCE 202000L
#define __USE_MISC 1
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "libalternatives.h"
#include "parser.h"

#if !defined(ALTERNATIVES_PATH)
#error "ALTERNATIVES_PATH is undefined"
#endif

#if !defined(CONFIG_DIR)
#error "CONFIG_DIR is undefind"
#endif

int loadDefaultAlternativesForBinary(const char *binary_name, struct AlternativeLink **alternatives)
{
	int configdirfd = open(CONFIG_DIR, O_DIRECTORY, O_RDONLY);
	if (configdirfd < 0)
		return -1;

	int binaryconfigdirfd = openat(configdirfd, binary_name, O_DIRECTORY, O_RDONLY);
	if (binaryconfigdirfd < 0) {
		int saved_error = errno;
		close(configdirfd);
		errno = saved_error;
		return -1;
	}

	close(configdirfd);

	DIR *dir = fdopendir(dup(binaryconfigdirfd));
	if (dir == NULL) {
		int saved_error = errno;
		close(binaryconfigdirfd);
		errno = saved_error;
		return -1;
	}

	struct dirent *dirptr;
	int prio = 0;
	const char *filename = NULL;

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
					int saved_error = errno;
					closedir(dir);
					errno = saved_error;
					return -1;
				}

				if (S_ISREG(info.st_mode))
					break;
			}
			// fallthrough
			default:
				continue;
		}

		int new_prio = atoi(dirptr->d_name);
		if (new_prio > prio) {
			prio = new_prio;
			free((void*)filename);
			filename = strdup(dirptr->d_name);
		}
	}
	if (errno != 0) {
		int saved_error = errno;
		closedir(dir);
		errno = saved_error;
		free((void*)filename);
		return -1;
	}
	closedir(dir);

	if (filename == NULL) {
		*alternatives = NULL;
		return 0;
	}

	int fd = openat(binaryconfigdirfd, filename, 0, O_RDONLY);
	if (fd < 0) {
		int saved_error = errno;
		free((void*)filename);
		close(binaryconfigdirfd);
		errno = saved_error;
		return -1;
	}
	free((void*)filename);
	close (binaryconfigdirfd);

	struct ParserState *state = initParser();
	char buffer[1024];
	int len = 0;
	int ret = 0;
	while (ret == 0 && (len = read(fd, buffer, 1024)) > 0)
		ret = parseConfigData(buffer, len, state);

	close(fd);
	*alternatives = doneParser(prio, state);
	return ret;
}