CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task21

task21: lab21.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task21
