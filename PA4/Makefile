#makefile for the server side interface

#calling the gcc compiler to build the code
all: webproxy.c
	gcc webproxy.c -lcrypto -lssl -o webproxy

#clean up the executable created
clean:
	rm -rf webproxy host_ip_map.txt *.html
