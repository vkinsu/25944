CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task10

task10: task10.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task10
