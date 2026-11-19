CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task2

task2: task2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task2
