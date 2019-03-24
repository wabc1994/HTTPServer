# a simple HTTP Server
a simple web server  written in c++ , parse GET,POST,HEAD,HTTP request, asynchronous logging buffer, keep-alive TCP.

- Reactor + NonBlocking I/O and EPOLL
- Timer to deal with long-time  invalid connection
- support thread pool,the main thread for create new connection, and then Assignment the job to the thread pool
- eventfd thread event notify 
- RAII small-point and MutexGuradlock
- openssl + http for https
- webbench test the server performance

## Compile and run

``` 
mkdir build
cd build
cmake ..
make
cd ..

```
