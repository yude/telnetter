CC=cc
RM=rm -f

.PHONY: all clean

all: telnetter

telnetter: main.c
		$(CC) -o telnetter main.c

clean:
	$(RM) telnetter
