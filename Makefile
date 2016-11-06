CC     = gcc
LD     = $(CC)
OPENMP = -fopenmp
CFLAGS = -Wall -pedantic -ggdb -O3 $(OPENMP)
LDFLAGS = $(OPENMP)

life:	life.o
	$(LD) $(LDFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f life.o life
