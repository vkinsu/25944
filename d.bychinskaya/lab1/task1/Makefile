CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task1

task1: task1.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task1
