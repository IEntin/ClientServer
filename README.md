Copyright (c) 2021 Ilya Entin.

### Fast Lockless Linux Clent-Server on Named Pipes.

Transport layer is named pipes (fifo, better performance and arguably stronger security than in case of sockets).

Lockless. Processing batches of requests without locking.

Memory Pooling. Business code and mostly compression/decompression are not allocating.

Built in optional LZ4 compression.

Business logic, compression, task multithreading, and transport layer are completely decoupled.

Business logic here is one of coding challenges I once had to work on. That exercize did not require multithreading, IPC, compression or optimization. This logic finds keywords in a request and performs financial calculations based on the results of this search. There are 10000 requests in a batch, each of these requests is compared with 1000 entries from another document containing keywords, money amounts and other information. The single feature of this code used in other parts of the application is a signature of the function taking request string_view as a parameter and returning the response string. Different business logic from a different field, not necessarily finance, can be plugged in. 

A different transport layer, e.g. tcp, can be plugged in as well without changes in other parts of the software.

In order to measure performance of the system the same batch was repeated in an infinite loop. I was mostly interested in the server performance, so requests from the client were compressed once and then repeatedly sent to the server. Of course, the server was processing these batches from scratch in each iteration. Processing of one batch takes 26 to 28 milliseconds on a rather old laptop, the client command being './client > /dev/null' to exclude printing which is doubling the latency. It would be interesting to test this system on a more advanced computer with large number of cores.

To test the code:

1. clone the repository

2. build the app\
make -j2\
The compiler must support c++20\
I use\
gcc  11.1.0 and\
clang 12.0.0

g++ is used by default. To use clang add CMPLR=clang++ to the make command.\
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

Names of pipes should match on server and clients. ProgramOptions.json contains\
lists for 5 clients, but this number can be increased, only performance can put the\
limit. The server was tested with 5 clients.

FIFO files are created on server startup and removed on server shutdown with Ctrl-C.\
By design, the clients should not create or remove FIFO files. The code does not do\
this, besides fifo directory permissions do not allow this.\
........

To start:\
on server side './server' in the project root\
on client side './client' in the testClient or any other directory.

No special requirements for the hardware.\
Using laptop with 4 cores and 4GB RAM to run the server and up to 5 clients.\
Running Linux Mint 20.2.

Multiple runtime options are in ProgramOptions.json files for the server and client
in corresponding directories.

Some of these:

"ProcessRequestMethod" : ["Log", "Echo", "Test"]\
'Log' is a business logic.\
'Echo' allows to test multithreading, compression, and fifo. To check the results save client\
output to the file and diff this file with the source (default is 'requests.log').\
'Test' returns simple calculations of the string lengths. The positive result is the absence of assertions.

To enable compression (default)\
"Compression" : "LZ4"\
If this is empty or something different the compression is disabled.

"TestCompression" - works with any other options. Asserts when original is not the same\
as compressed and subsequently decompressed.

"Timing" prints elapsed times between selected lines of the code. Currently it shows client latency\
for the batch of 10000 requests and the total run time for the server. See Chronometer class in the code.

"DYNAMIC_BUFFER_SIZE" is the size of the memory pool entries.\
This parameter controls the task size(the number of requests in the batch) and\
memory footprint of the application. The latter is important for embedded systems.

"Diagnostics" shows details of all stages of business calculations for each request.

=======
### Linux Client-Server
Transport layer based on named pipes.\
Lockless. Processing batches of requests  without locking.\
Business logic, tasks multithreading, and transport layer are completely decoupled.\
Memory pooling. Business logic, compression and most of fifo are not allocating.\
Business logic here is a coding challenge I once worked on.\
It can be replaced with any other batch processing from a different field, not necessarily financial.
