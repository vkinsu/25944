CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task5

task5: task5.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task5
