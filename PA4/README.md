README for my web proxy server implementation.

I have desiged a web proxy server which can take in multiple client requests and serve them by virtue of having a multi-process programming model.
I have made use of fork calls to handle multiple incoming client requests.

Also implemented is a caching feature which keeps a record/cache of the recently accessed web content for a certain user-configurable timeout value.
In case a request for the same page is made within the timeout, the proxy directly serves the requested content rather than going all the way through the server.
A list of ip address and domain name pairing has been implemented so that the number of DNS requests can be reduced by directly looking up the matching pair. 
There is also a blocking feature implemented which blocks access to certain websites mentioned in a blocking list.

How to run:-

Example calls:
$./webproxy 9999 20
(./webproxy - name of program
9999 - example port no.
20 - cache timeout in second
)

On another terminal window:
$telnet 127.0.0.1 9999
- GET http://shpa.com HTTP/1.1
- GET http://umich.edu HTTP/1.1
- GET http://grad.berkeley.edu HTTP/1.1

For working with google chrome:
$google-chrome --proxy-server="127.0.0.1:9999"
