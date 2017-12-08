README for my programming assignment 4.

How to run:-

Example calls:
$./webproxy 9999

On another window:
$telnet 127.0.0.1 9999
- GET http://shpa.com HTTP/1.1
- GET http://umich.edu HTTP/1.1
- GET http://grad.berkeley.edu HTTP/1.1

For working with google chrome:
$google-chrome --proxy-server="127.0.0.1:9999"
