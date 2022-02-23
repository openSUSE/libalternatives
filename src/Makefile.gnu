SOURCES = libalternatives.c options_parser.c config_parser.c
HEADERS = libalternatives.h
# Parse version from CMakeLists.txt and set SOVERSION to be its major part
VERSION = $(shell /usr/bin/grep 'project.libalternatives VERSION' ../CMakeLists.txt | /usr/bin/awk '{print $$3}')
SOVERSION = $(word 1, $(subst ., ,$(VERSION)))
LIBRARY_BASENAME = libalternatives.so
LIBRARY = $(LIBRARY_BASENAME).$(VERSION)
OBJS = $(SOURCES:.c=.o)

ETC_PATH ?= /etc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(LIBDIR)/lib
DATADIR ?= $(PREFIX)/share
CONFIG_DIR ?= $(DATADIR)/libalternatives
CONFIG_FILENAME ?= libalternatives.conf
CFLAGS += -DCONFIG_DIR=\"$(CONFIG_DIR)\" -DETC_PATH=\"$(ETC_PATH)\" -DCONFIG_FILENAME=\"$(CONFIG_FILENAME)\" -fPIC -fvisibility=hidden -Wall -Wextra -Wpedantic -std=gnu99 -fvisibility=hidden

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	gcc $(LDFLAGS) -fPIC -shared -Wl,--version-script,libalternatives.version -Wl,-soname,libalternatives.so.$(SOVERSION) $(OBJS) -o $(LIBRARY)
	ln -s libalternatives.so.$(VERSION) libalternatives.so.$(SOVERSION)
	ln -s libalternatives.so.$(SOVERSION) libalternatives.so

install: $(LIBRARY)
	install -d -t $(DESTDIR)$(LIBDIR) libalternatives.so.$(VERSION) libalternatives.so.$(SOVERSION) libalternatives.so

clean:
	rm -f $(OBJS)
	rm -f $(LIBRARY)
	rm -f $(LIBRARY_BASENAME).$(SOVERSION)
	rm -f $(LIBRARY_BASENAME)
