# build file for xml parser

CC=gcc
LIBS= -lexpat

# --------------------------------- #
parse_wp : parse_wp.o
	$(CC) -o parse_wp parse_wp.o $(LIBS)

parse_wp.o : parse_wp.c
	$(CC) -c parse_wp.c

# --------------------------------- #

.PHONY : clean
clean :
	rm -f *.o parse_wp
