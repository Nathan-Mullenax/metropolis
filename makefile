all: metropolis

metropolis: metropolis.cpp
	c++ -std=c++11 metropolis.cpp -lncurses -ometropolis
