### Fast Lockless Linux Clent-Server on Named Pipes.

Transport layer is named pipes (fifo, better performance and arguably stronger security than in a case of sockets).

Lockless. Processing batches of requests without locking.

Memory Pooling. Business code and mostly compression/decompression are not allocating.

Built in optional LZ4 compression.

Business logic, compression, task multithreading, and transport layer are completely decoupled.

Business logic here is just an example. It finds keywords from a request and does financial calculations based on the results of this search. There are about 10000 requests in a batch, each of these requests is compared with about 1000 lines of text from another document containing keywords, money amounts and other information. The single feature used in other parts of the application is a signature of the function taking request string_view as a parameter and returning the response string. Different business logic from a different field, not necessarily finance, can be easily plugged in.

A different transport layer, e.g. tcp, can be plugged in as well without changes in other parts of the software.

To test the code:

1. clone the repository

2. build the app\
make -j2\
The compiler must support c++20\
I use\
gcc  11.1.0 and\
clang 12.0.0

by default g++ is used. To use clang add CMPLR=clang++ to the make command.\
There are options for sanitizers (address, undefined, leak or thread), profiler, different OPTIMIZATION levels.\
see the makefile.

3. run

Change settings for FIFO directories and file names on server and clients in ProgramOptions.json:

server:\
  "FifoDirectoryName" : <created directory name, anywhere with appropriate permissions>,\
  "ReceiveIds" : "Receive1 Receive2 Receive3 Receive4 Receive5",\
  "SendIds" : "Send1 Send2 Send3 Send4 Send5"

client #1:\
  "FifoDirectoryName" : <the same as for the server>,\
  "SendId" : "Receive1",\
  "ReceiveId" : "Send1",

Names of pipes should match on server and clients.

FIFO files are created on server startup and removed on server shutdown with Ctrl-C.\
Clients are not allowed to create or remove FIFO files.\
........

To start:\
on server side './server' in the project root\
on client side './client' in the testClient subdirectory or another directory

No special requirements for the hardware.\
Using Lenovo laptop with 4 cores and 4GB RAM to run the server and up to 5 clients.\
Running Linux Mint 20.2

Multiple runtime options are in ProgramOptions.json files for the server and client
in corresponding directories.

Some of these:

"ProcessRequestMethod" : ["Log", "Echo", "Test"]\
'Log' is a business logic.\
'Echo' is self explanatory. To check the results save client output to the file\
and diff this file with the source (default 'requests.log').\
'Test' returns simple calculations of the string lengths. Result is absence of assertions.

To enable compression (default)\
"Compression" : "LZ4"\
If this is empty or something different the compression is disabled.

"TestCompression" - works with any other options. Asserts when original is not the same\
as compressed and subsequently decompressed.

"Timing" prints elapsed times between selected expressions. Currently it shows client latency\
for the batch of 10000 requests. See Chronometer class in the code.

"DYNAMIC_BUFFER_SIZE" is the size of the memory pool entries.\
The BUFFER_SIZE is selected so that compressed payload does not exceed default\
pipe size which is typically 65,532 bytes. Under this condition writing and reading\
to/from fifo happens in one shot, or atomically as documentation states this. Buffer\
can grow automatically if necessary, e.g. if reply is larger then request or compression is disabled,\
breaking atomicity with small performance cost.

"Diagnostics" shows details of all stages of business calculations for each request.
 
=======
### Linux Client-Server
Transport layer based on named pipes.\
Lockless. Processing batches of requests  without locking.\
Business logic, tasks multithreading, and transport layer are completely decoupled.\
Memory pooling. Business logic and compression are not allocating.\
Business logic here is an example I once worked on.\
It can be replaced with any other batch processing from a different field, not necessarily financial.\
