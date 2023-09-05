CC=cc
RM=rm -f

all: telnetter

telnetter: main.c
		$(CC) -o telnetter main.c

.PHONY: clean
clean:
	$(RM)
