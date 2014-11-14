CC     = gcc
LD     = $(CC)
CFLAGS = -ggdb -O3 # -fopenmp

life:	life.o
	$(LD) -o $@ $^

.PHONY: clean

clean:
	rm -f life.o life
