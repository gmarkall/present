CFLAGS = -std=c99 -Wall -Werror -Wextra -pedantic

all: present

present: present.c
	gcc $(CFLAGS) -o present present.c

.PHONY: clean

clean:
	rm -f present
