CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task9

task9: task9.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task9
