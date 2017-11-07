CFLAGS = -std=c99 -Wall -Werror -Wextra -pedantic
PROGS = present present_bitslice


all: $(PROGS)

%: %.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f $(PROGS)
