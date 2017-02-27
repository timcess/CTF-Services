CFLAGS += -Wall
all: clean poetry
poetry: poetry.c database.c
	gcc -o poetry poetry.c database.c $(CFLAGS)
clean:
	rm -f poetry
.PHONY: all clean
