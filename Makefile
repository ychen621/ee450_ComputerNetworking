clean:
	-rm -f client serverM serverA serverB

all: serverA.cpp serverB.cpp serverM.cpp client.cpp
	g++ -o serverA serverA.cpp

	g++ -o serverB serverB.cpp

	g++ -o serverM serverM.cpp
	
	g++ -o client client.cpp	

.PHONY: serverA
serverA: 
	./serverA

.PHONY: serverB
serverB:
	./serverB

.PHONY: serverM
serverM:
	./serverM

.PHONY: client
client:
	./client