SOURCES = alternatives.c group_consistency_rules.c list_binaries.c
HEADERS = libalternatives.h
ALTS_binary = alts
OBJS = $(SOURCES:.c=.o)

include ../src/Makefile.gnu.common

CFLAGS += -fpie

all: $(ALTS_binary)

../src/libalternatives.so:
	$(MAKE) -C ../src -f Makefile.gnu

$(ALTS_binary): $(OBJS) ../src/libalternatives.so
	gcc $(LDFLAGS) $(OBJS) -o $(ALTS_binary) ../src/libalternatives.so

install: $(LIBRARY)
	install -d -t $(DESTDIR)$(BINDIR) $(ALTS_binary)

clean:
	rm -f $(OBJS)
	rm -f $(ALTS_binary)

