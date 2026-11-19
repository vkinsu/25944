CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task6

task6: task6.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task6
