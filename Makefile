CFLAGS = -ggdb -O3 # -fopenmp

life:	life.o
	gcc -o $@ $^

.PHONY: clean

clean:
	rm -f life.o life
