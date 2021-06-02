CXX = g++
FLAGS = -O3

all: calculator

calculator: calculator.cpp
	$(CXX) $(FLAGS) $< -o $@

clean:
	rm calculator