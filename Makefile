main: main.cpp
	g++ -g main.cpp -o sched -std=c++17

clean:
	rm -f sched *~
