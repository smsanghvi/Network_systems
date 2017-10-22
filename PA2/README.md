This project is an implementation of a webserver which can handle multiple incoming requests from any web browser - anywhere in the world.
Using a server configuration file wherein you feed in your server port number to listen on, your document root directory having all the content to be rendered, a default web page to be displayed in case of an incorrect request, content types being handled by the server and a connection keep alive timeout.

Github link for source code: https://github.com/smsanghvi/Network_systems/tree/master/PA2

The server runs in an infinite loop (always on, unless terminated), and the web browser can connect to the server if it knows the ip address and port number that the server is listening on. Using the TCP model of reliable connection, the server first calls socket() to setup a socket first, bind() to bind the socket number to the particular port, listen() for incoming connections, then run an infinite while loop calling the blocking accept() function to accept incoming requests. Once a request is accepted, it creates a new process using fork() and the request is handled by a new child process. This is how multiple requests can be handled -> by forking and having a new process handle a request. Finally, using send() and recv() functions, data can be sent and received over the socket.


Features:
- Parsing of ws.conf configuration file to extract port number, root directory, default index page, file format types supported, timeout value
- Implemented functionality for GET: extracted the request method from browser request, and sent back the file if present on the server with a 200 status code of success
- If the requested file is not present in the directory, return a 404 status code of file not found
- If requested method is POST, you need to use telnet to test this and enter the content to be seen in the server response in addition to the requested file
- If requested version is anything apart from HTTP/1.0 or HTTP/1.1, then return a 400 error code back to the client - indicating bad request version
- If the requested method is anything apart from GET and POST, return a 501 not implemented code indicating that the requested method (HEAD, DELETE, OPTIONS, etc) has not been implemented.
- Anything apart from these cases is returned with a 500 status code implying that it is an internal server error - not something which is wrong on the client's end.


How to run the code:
> The user first has to populate the ws.conf server configuration file for his specific use case
> Compile the source code by running $make
> Start the server by ./server

>To test for successful GET:
From your web browser, type in "http://localhost:PORT/images/apple_ex.png" and you should see the image of an apple.

>To test for default case:
From your web browser, type in "http://localhost:8888/" and you should see the default index.html file.

>To test for unsuccessful GET and error code for 404, type in some link not present, for example, "http://localhost:8888/images/random.png". You should see a 404 - Resource not found page rendered.

>To test for POST: 
On client side, type in -
$telnet 127.0.0.1 8888
$POST /test.html HTTP/1.1\r\nConnection: Keep-alive\r\n\r\n<html><body><pre><h1>POSTDATA</h1></pre>This is a POST message</body></html>

You should a server response having the POSTDATA header and the message you typed in following it followed by the requested html file requested.

>To test for unsuccessful GET and error code of 400, type in an invalid http version(like HTTP/1.5) on client side, for example, go on to telnet and type-
$telnet 127.0.0.1 8888
$POST /test.html HTTP/1.5\r\nConnection: Keep-alive\r\n\r\n<html><body><pre><h1>POSTDATA</h1></pre>This is a POST message</body></html>

You should see a 400 error page in return.

>To test for error code of 501, type in a not supported request method like HEAD on client side, for example, go on to telnet and type-
$telnet 127.0.0.1 8888
$HEAD /test.html HTTP/1.1\r\nConnection: Keep-alive\r\n\r\n<html><body><pre><h1>POSTDATA</h1></pre>This is a POST message</body></html>

You should see a 501 not implemented error page in return.

>Any other condition apart from these is a 500 internal server error condition and the appropriate 500 page is returned.


(EXTRA CREDIT) Pipelining: In order to support HTTP/1.1 functionality, implemented a persistent connections feature and pipelined client requests. After the result of a single request has been returned, the server has its socket open for a certain value of 'timeout', and if a request is received within that period to update the timeout value and service the request. For testing purposes, I set the timeout to a value of 10 seconds in my server configuration. Using the signal() function and setting the timeout value using alarm(), I was able to enter a signal handler after 10 seconds and turn off a volatile global flag to indicate to the main code that a timeout has occured. In this case, the loop will be exited after servicing the ongoing request. 


(EXTRA CREDIT) Support for POST method: Handled POST requests and sent back the POST DATA along with the requested file to the client.

Using a combination of these strategies, I was able to implement a pipelined webserver capable of servicing multiple incoming browser requests.
