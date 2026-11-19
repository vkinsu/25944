CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task3

task3: task3.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task3
