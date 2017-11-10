This project is an implementation of the client-server model of communication using UDP protocol. Technically, you could be running your server code on a server and communicate with it from any part of the world, only given the server ip and the port number on which it is listening.

Github link for source code: https://github.com/smsanghvi/Network_systems/tree/master/PA1 

There is a client and a server running in an infinite loop (always on, unless terminated), and the user is provided with a host of options on the client end. A socket is first created on the server and is bound to a particular port. A client is now eligible to communicate with the server if it has the port no. and server ip address. Using the sendto and recvfrom functions, data can be either sent to or received from a particular machine. The recvfrom function is a blocking function by default, unless using the setsockopt function to change certain socket parameters.

The user can select one of 5 possible options:
1> get <FILE> : The client requests a file from the server. The server will first confirm whether such a file exists in its directory and send a confirmation about the same to the client. It is only then that it starts sending the file to the client, packet-by-packet.

2> put <FILE> : The client sends a file to the server. The server will confirm whether it is ready to accept the file and then the client starts transmitting the packets on a packet-by-packet basis.

3> delete <FILE> : The client requests that a particular file be deleted on the server end. The server first checks whether it has the requested file, and if t does it deletes it using the 'remove' function. It then sends a message to the client stating whether the operation was successful or not. 

4> ls : This function basically returns a list of all the files present in the server's directory back to the client. Think of it as a menu of the files that the server has so that the client can decide whta further action it needs to take (get file, delete file or retrive file)

5> exit : This function requests the server to shut down gracefully by issuing a break signal and closing the socket descriptor.


How to run the code:
In the server folder, run 
>make to compile the source file, and a 
> ./udp_server <port> to run the server code
In the client folder, run
>make to compile the source file, and a 
> ./udp_client <server-IP> <port> to run the client implementation 
The way this interface can be used is the following:
The client and server are running in an infinite loop - 
1> get foo1 : To get a file 'foo1' from server to client
2> put foo1 : To send a file 'foo1' from client to server
3> delete foo1 : To delete file 'foo1' on the server
4> ls : To list all the files present in the server directory
5> exit : To cause the server to shutdown gracefully

Reliability: A major part of the codebase developed has to do with implementing a certain degree of reliability into the UDP protocol. I do understand that there is a TCP protocol available for implementing reliability, but encoding reliability into UDP would be a very interesting development experience. 

I used the Stop and Wait AQR algorithm which essentially has the sender send a packet to the receiver, the receiver acknowledges the packet and then the sender sends the next packet on receipt of ACK of previous packet. The setsockopt function allows you to change the timeout for the receive function after which I ask for a retransmission if an ACK has not been received. If the receiver recieves a packet that it has already acknowledged, it just needs to send a ACK for that packet again. Using this method of ACKs and retransmission of non-acknowledged packets, a sufficiently reliable UDP connection can be created.

I have tested the algorithms with files ~15MB big and the file transfer proved to be successful.
   
(EXTRA CREDIT) Encryption: I have implemented an encryption algorithm which encrypts all the data within a packet before sending it over to the receiver which decrypts the content to extract the data. I implemented a XOR encryption strategy which made use of a 64-character key to encrypt the data message before sending it over. On the receiver end, the message is XORed again with the same key to get back the original message.

Using a combination of strategies involving implenting encryption and reliability over the UDP link connection, I implemented a functional UDP application.
