Testing for POST:
On client side->
$telnet 127.0.0.1 8888
$POST /test.html HTTP/1.1\r\nConnection: Keep-alive\r\n\r\n<html><body><pre><h1>POSTDATA</h1></pre>This is a POST message</body></html>
