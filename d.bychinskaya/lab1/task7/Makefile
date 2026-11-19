CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task7

task7: task7.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task7
