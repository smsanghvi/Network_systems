This project is an implementation of a distributed file system that can allow the client to store and retrieve files on multiple servers. Each file is divided into pieces and stored on different servers which ensures that the files can be retrieved even if one of the servers is down.

There is a client that uploads and downloads files onto and from 4 servers : DFS1, DFS2, DFS3 and DFS4. In my case in order to test the concept, I ran the 4 servers on the same machine with 4 different port numbers. 

The following features have been implemented:
- LIST : Calling LIST on the client lists all the partial files listed on the 4 servers and also reports back the status of the files present on them. If all the 4 partial files can be found across the servers, then a status of COMPLETE file is printed, else INCOMPLETE file is printed.
- PUT : The PUT functionality is called whenever the client wants to upload content to the server. It splits the file content into 4 parts and uploads these partial files to the 4 servers based on a hashing scheme described below.
- GET : The GET functionality is called to pull in partial files from the 4 servers onto the client and merge them into a complete file. There is a dual redundancy introduced so that all the partial files can be retrieved and combined even if one of the servers is down.

Running the code:-
- Open 5 terminal windows on your Linux system - 1 for client and 4 for 4 server instances
- Compile the client and server sides by running make
- Run the client from inside the DFC folder by: ./dfclient dfc.conf
- Run 4 server instances as follows:
  > ./dfserver /DFS1 10001
  > ./dfserver /DFS2 10002
  > ./dfserver /DFS3 10003
  > ./dfserver /DFS4 10004

A more in-depth explanation of the functionality: -
- There is a configuration file on the client side which has a record of the 4 server IP addresses and port numbers it needs to connect to, its username and password. This configuration file is parsed by the client to extract all of the data.
- The server also maintains a configuration file that keeps a record of various username - password combos and verifies the information received from the client before sending out data.
- Each LIST, GET and PUT command is accomadated by a valid username and password, else the server responds back with an 'Invalid Username/Password combo' error message. There is directory handling involved which ensures that a folder (with the same name as username) is created within the DFS server folder. This folder will store a bunch of part files from different files.
- LIST : Inquires what files are stored on the DFS servers and returns the filenames along with a message saying whether this is a complete file or partial file. 
- PUT : There is a scheme followed to upload data onto the servers. Based on the MD5 hash value of the file content, we need to send a particular pair of file parts to every server. This ensures a redundancy in our system which will make sure that the file can be retrieved even if one server is down.
- GET : All the individual file parts are sent from the 4 servers to the client (8 parts in total). The client reads in all of these parts and then ignores parts that have the correct content and have been read in twice. The 4 parts are then merged to give a single file.
- This approach ensures a greater reliability through redundancy.
- There is greater privacy ensured through encryption of file content which is in turn decrypted on the server end.
- There are timeouts implemented on the client end and the client will check for 1 second if a server is up and move on if it isn't.
