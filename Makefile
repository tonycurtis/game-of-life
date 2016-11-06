CC       = gcc
LD       = $(CC)
OPENMP   = -fopenmp
DEBUG    = -ggdb
OPTIMIZE = -O3
LDFLAGS  = $(DEBUG) $(OPTIMIZE) $(OPENMP)
STRIDENT = -Wall -pedantic
CFLAGS   = $(STRIDENT) $(LDFLAGS)

life:	life.o
	$(LD) $(LDFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f life.o life
