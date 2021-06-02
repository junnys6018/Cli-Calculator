CXX = g++
FLAGS = -O3 -std=c++17

all: calculator

calculator: calculator.cpp
	$(CXX) $(FLAGS) $< -o $@

pretty: 
	clang-format -i calculator.cpp

clean:
	rm calculator