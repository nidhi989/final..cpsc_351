
all: receiver sender

receiver: recvsig.cpp
	g++ -o recvsig recvsig.cpp

sender: sendersig.cpp
	g++ -o sendersig sendersig.cpp
