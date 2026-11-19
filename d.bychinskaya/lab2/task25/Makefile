CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task25

task25: lab25.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task25
