SOURCES = libalternatives.c options_parser.c config_parser.c
HEADERS = libalternatives.h

include Makefile.gnu.common

CFLAGS += -fPIC

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
