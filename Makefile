kilo: kilo.c common.h highlight.o highlight.h
	$(CC) kilo.c highlight.o -o kilo -Wall -Wextra -pedantic -std=c99
hightlight.o : common.h highlight.c highlight.h
	$(CC) -c hightlight.c  -Wall -Wextra -pedantic -std=c99
