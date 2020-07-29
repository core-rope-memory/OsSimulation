CXX = g++
CXXFLAGS = -Wall -g -pthread

.cpp : 
	$(CXX) $(CXXFLAGS) -o $* $*.cpp -std=c++11

OsSim: CycleTime.h Configuration.h MetaCommand.h Process.h ReadyQueue.h MetaData.h OSprocessRunner.h OsSim.o
	$(CXX) $(CXXFLAGS) -o $@ $@.o -std=c++11

OsSim.o: CycleTime.h Configuration.h MetaCommand.h Process.h ReadyQueue.h MetaData.h OSprocessRunner.h OsSim.cpp
	$(CXX) $(CXXFLAGS) -c $*.cpp -std=c++11