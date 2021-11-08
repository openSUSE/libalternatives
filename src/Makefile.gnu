SOURCES = libalternatives.c options_parser.c config_parser.c
HEADERS = libalternatives.h
SOVERSION=1
LIBRARY = libalternatives.so.$(SOVERSION)
OBJS = $(SOURCES:.c=.o)
CFLAGS += -fPIC -fvisibility=hidden -Wall -Wextra -Wpedantic -std=gnu99

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	gcc $(LDFLAGS) -shared -Wl,--version-script,libalternatives.version -Wl,-soname,libalternatives.so.$(SOVERSION) $(OBJS) -o $(LIBRARY)
	ln -s libalternatives.so.$(SOVERSION) libalternatives.so

clean:
	rm -f $(OBJS)
	rm -f $(LIBRARY)
	rm -f libalternatives.so

