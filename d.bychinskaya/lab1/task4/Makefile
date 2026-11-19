CFLAGS ?= -O2 -Wall -Wextra

READLINE_PREFIX ?= $(shell brew --prefix readline 2>/dev/null)
READLINE_CFLAGS ?= $(if $(READLINE_PREFIX),-I$(READLINE_PREFIX)/include)
READLINE_LIBS ?= $(if $(READLINE_PREFIX),-L$(READLINE_PREFIX)/lib) -lreadline

.PHONY: all clean
all: task4

task4: task4.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(READLINE_CFLAGS) $(LDFLAGS) -o $@ $< $(READLINE_LIBS) $(LDLIBS)

clean:
	$(RM) task4
