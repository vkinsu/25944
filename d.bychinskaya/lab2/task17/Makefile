CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task17

task17: lab17.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task17
