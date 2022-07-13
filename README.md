# CMPT300-a3
Assignment 3 Our new chatbot – Lets-talk
Multi-threaded program for bidirectional communication between different terminals on the same network. Uses UNIX UDP, IPC, and the client/server model.

How to Run
# Clean and build executable
$ make
# Output "s-talk" will be produced. Example to run:
# Example format ./s-talk [port #] [remote machine name] [remote port #]
$ ./s-talk 7000 localhost 8000 . Do this in two different terminals to see the messages
# Then on another terminal run:
$ ./s-talk 8000 localhost 7000 
# Enter ! to exit
Features
Can send messages 
Sending ! ends session on both terminals
