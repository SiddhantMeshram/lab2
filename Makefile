DIR=src

sched: $(DIR)/take_input.o $(DIR)/main.o $(DIR)/des.o $(DIR)/event.o $(DIR)/fcfs.o $(DIR)/lcfs.o $(DIR)/pre_prio.o $(DIR)/prio.o $(DIR)/process.o $(DIR)/round_robin.o $(DIR)/scheduler.o $(DIR)/srtf.o
	g++ --std=c++17 -o sched $(DIR)/take_input.o $(DIR)/main.o $(DIR)/des.o $(DIR)/event.o $(DIR)/fcfs.o $(DIR)/lcfs.o $(DIR)/pre_prio.o $(DIR)/prio.o $(DIR)/process.o $(DIR)/round_robin.o $(DIR)/scheduler.o $(DIR)/srtf.o

$(DIR)/take_input.o: $(DIR)/take_input.cpp
	g++ -c $(DIR)/take_input.cpp --std=c++17 -o $(DIR)/take_input.o

$(DIR)/main.o: $(DIR)/main.cpp $(DIR)/main.h
	g++ -c $(DIR)/main.cpp --std=c++17 -o $(DIR)/main.o

$(DIR)/fcfs.o: $(DIR)/fcfs.cpp $(DIR)/fcfs.h
	g++ -c $(DIR)/fcfs.cpp --std=c++17 -o $(DIR)/fcfs.o

$(DIR)/lcfs.o: $(DIR)/lcfs.cpp $(DIR)/lcfs.h
	g++ -c $(DIR)/lcfs.cpp --std=c++17 -o $(DIR)/lcfs.o

$(DIR)/srtf.o: $(DIR)/srtf.cpp $(DIR)/srtf.h
	g++ -c $(DIR)/srtf.cpp --std=c++17 -o $(DIR)/srtf.o

$(DIR)/round_robin.o: $(DIR)/round_robin.cpp $(DIR)/round_robin.h
	g++ -c $(DIR)/round_robin.cpp --std=c++17 -o $(DIR)/round_robin.o

$(DIR)/prio.o: $(DIR)/prio.cpp $(DIR)/prio.h
	g++ -c $(DIR)/prio.cpp --std=c++17 -o $(DIR)/prio.o

$(DIR)/pre_prio.o: $(DIR)/pre_prio.cpp $(DIR)/pre_prio.h
	g++ -c $(DIR)/pre_prio.cpp --std=c++17 -o $(DIR)/pre_prio.o

$(DIR)/scheduler.o: $(DIR)/scheduler.cpp $(DIR)/scheduler.h
	g++ -c $(DIR)/scheduler.cpp --std=c++17 -o $(DIR)/scheduler.o

$(DIR)/des.o: $(DIR)/des.cpp $(DIR)/des.h
	g++ -c $(DIR)/des.cpp --std=c++17 -o $(DIR)/des.o

$(DIR)/event.o: $(DIR)/event.cpp $(DIR)/event.h
	g++ -c $(DIR)/event.cpp --std=c++17 -o $(DIR)/event.o

$(DIR)/process.o: $(DIR)/process.cpp $(DIR)/process.h
	g++ -c $(DIR)/process.cpp --std=c++17 -o $(DIR)/process.o

clean:
	rm -f $(DIR)/*.o sched