hellocator: hellocator.cpp libhelp.a
	g++ -o hellocator hellocator.cpp -std=c++11 -lpthread -lsqlite3 libhelp.a -ggdb -g3

libhelp.a: help.c help.h
	gcc -c help.c
	ar rcs libhelp.a help.o

clean:
	rm *.o

rm:
	rm db.db hellocator
	rm -rf usersSpaces
