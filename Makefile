CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

all: code

code: main.cpp
	$(CXX) $(CXXFLAGS) -o code main.cpp

cheat: cheat.cpp
	$(CXX) $(CXXFLAGS) -o cheat cheat.cpp

anticheat: anticheat.cpp
	$(CXX) $(CXXFLAGS) -o anticheat anticheat.cpp

clean:
	rm -f code cheat anticheat cheat-submit.cpp anticheat-submit.cpp *.o
