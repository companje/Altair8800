CC=gcc
CFLAGS=-I. -Ilocal $(OPTFLAGS)
LIBS=$(OPTLIBS)
SOURCES=local/main.cpp i8080.c local/i8080_hal.c
#$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

%.o: %.c $(SOURCES)
	$(CC) -c -o $@ $< $(CFLAGS)

altair8800: $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: all test clean local

clean:
	rm *.o local/*.o altair8800
