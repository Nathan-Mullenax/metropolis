all: metropolis met-det met-rand

metropolis: metropolis.cpp
	c++ -std=c++11 metropolis.cpp -lncurses -O -ometropolis

met-det: gen-det.cpp
	c++ gen-det.cpp -O -omet-det

met-rand: write-met.cpp
	c++ write-met.cpp -O -omet-rand

clean:
	rm ./metropolis ./met-det ./met-rand
