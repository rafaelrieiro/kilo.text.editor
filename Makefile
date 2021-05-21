kilo: kilo.c common.h highlight.o hllines.o
	$(CC) kilo.c highlight.o hllines.o -o kilo -Wall -Wextra -pedantic -std=c99
hightlight.o : common.h highlight.c highlight.h hllines.o 
	$(CC) -c hightlight.c hllines.o -Wall -Wextra -pedantic -std=c99
hllines.o    : hllines.c hllines.h common.h
	$(CC) -c hllines.c -Wall -Wextra -pedantic -std=c99
