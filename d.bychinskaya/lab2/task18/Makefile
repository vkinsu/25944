CFLAGS ?= -O2 -Wall -Wextra

.PHONY: all clean
all: task18

task18: lab18.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) task18
