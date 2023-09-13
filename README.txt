a. Full Name: Yi-Hsuan Chen
b. Student ID: 3858722280
c. What I have done in the assignment?
	I have implemented a system that can assist the user to find the intersection of time available from the meeting participants. The system consists of two backend servers, a main intermediate server, and an interactable client program. The backend servers (Server A and Server B) can read and store the file that contains the user name list and their time availability. Furthermore, after receiving the participants list from the main server, the backend servers calculate the intersection of available time slots and send the result back to the main server. The main intermediate server (Main Server) stores the name list sent from Server A and Server B and receives the errorless participants (no invalid characters) list from the client. Then the main server classifies the names in the participants' list to determine whether the name resides in Server A or Server B or the name does not exist in the system. Next, if the name list contains names that do not exist in the system, the main server sends the "not exist" list to the client. Otherwise, the main server sends the name list to Server A and/or Server B to obtain the intersection of time availability. When the main server receives the results back from Server A/B, it intersects the results and sends the final result to the client. The client reads the input from the user and conducts the error-checking to make sure the input contains small letters only. Then the client sends the input which is the participants list to the main server to request obtaining the intersection of time available from the meeting participants.
d. Code files:
	1. serverA.cpp:
		- Read the namelist and the available time slots of the people in the namelist from a.txt
		- Create UDP socket to communicate with Main Server (Port number: 21280)
		- Send the namelist to Main Server
		- Receive the participants that resides in a.txt from Main Server
		- Run the intersect algorithm to find out the intersection of available time from participants resided in a.txt
		- Send the intersection result back to Main Server

	2. serverB.cpp
		- Read the namelist and the available time slots of the people in the namelist from b.txt
		- Create UDP socket to communicate with Main Server (Port number: 22280)
		- Send the namelist to Main Server
		- Receive the participants that resides in b.txt from Main Server
		- Run the intersect algorithm to find out the intersection of available time from participants resided in b.txt
		- Send the intersection result back to Main Server

	3. serverM.cpp
		- Create UDP socket to communicate with Server A and Server B (Port number: 23280)
		- Receive and store the namelists from Server A and Server B
		- Create parent TCP socket to handshake with Client (Port number: 24280)
		- Accept the connection from client and create child socket the interact with Client
		- Receive the participants list from Client
		- Determine where the name resides in (Server A, Server B, or do not exist)
		- Send the intersection request (namelist) to Server A and Server B
		- Compute the final intersection and send the result back to Client

	4. client.cpp
		- Create TCP socket to communicate with Main Server (Port number is dynamically assigned)
		- Keep reading the input (participants list) from the user
		- Check the input format (only small letters are allowed, and the names are space-separated)
		- Send the errorless participants list to Main Server
		- Receive the final intersection result from Main Server
		- Report to the user

e. The format of all message exchanged:
	1. serverA.cpp and serverB.cpp
		- "namelist" sending to Main Server:
			------------------------------------------
			| name1,name2,... (no space after comma) |
			------------------------------------------
		- "time slots" sending to Main Server:
			----------------------------------------
			| val1,val2,... (no space after comma) |
			----------------------------------------
			Time slots for each person were transformed into integer vectors with the size of 101. The index represents time and the value represents the availability (0 = not available, 1 = available).
			e.g [[2, 5], [98, 99]] will be transformed into
				index: 0 1 2 3 4 5 6 7 ...... 96 97 98 99 100
				value: 0 0 1 1 1 1 0 0 ...... 0  0  1  1  0
				============================================================
				time slots sending to Main Server: 0,0,1,1,1,1,0...0,0,1,1,0

	2. serverM.cpp
		- "list of names that are not in A or B" sending to Client:
			------------------------------------------
			| f,name1,name2,... (no space after comma) |
			------------------------------------------
		- "namelist" sending to Server A or Server B:
			------------------------------------------
			| name1,name2,... (no space after comma) |
			------------------------------------------
		- "final intersection result" to Client:
			-----------------------------------------
			| [[start0, end0], [start1, end1], ...] |
			-----------------------------------------
		- "exist participant namelist" sending to Client:
			----------------------------------------------
			| name1, name2, ... (with space after comma) |
			----------------------------------------------
			
	3. client.cpp
		- "errorless namelist" sending to Main Server:
			------------------------------------------
			| name1,name2,... (no space after comma) |
			------------------------------------------

g. Idiosyncrasy of your project.
h. Reused code
	1. Beej’s Guide to Network Programming:
		- socket()
		- bind()
		- listen()
		- accept()
		- sendto()
		- send()
		- recvfrom()
		- recv()
	2. EE450 Project Instruction:
		- getsockname()


