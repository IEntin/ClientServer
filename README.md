Copyright (C) 2021 Ilya Entin.

### Fast Linux Lockless Clent-Server with FIFO and TCP clients

This server can work with multiple mixed tcp and fifo clients.

TCP communication layer is using boost Asio library.

Fifo provides better performance and arguably stronger security than tcp.\
If fifo is not an option tcp client can connect to the server without stopping it.\
Due to protocol and predictable sequence of reads and writes, it is possible to make pipes bidirectional.\
This situation is unlike e.g. chat application or similar when unidirectional fifo is normally used.\
After write we close write file descriptor and open read fd for the same end of the pipe, and so on.\
This significantly simplified the setup - one fifo per client. See below fragments of configuration\
in ProgramOptions.json for the server and clients. Testing showed that performance impact of fd reopening\
is small for large batches when reopening is relatively rare.

Lockless. Processing batches of requests without locking.

Memory Pooling. Business code and compression/decompression are not allocating.

Builtin optional LZ4 compression.

Business logic, compression, task multithreading, and communication layers are completely decoupled.

Business logic here is an example of financial calculations I once worked on. This logic finds keywords in the request from another document and performs financial calculations based on the results of this search. There are 10000 requests in a batch, each of these requests is compared with 1000 entries from another document containing keywords, money amounts and other information. The easiest way to understand this logic is to look at the responses with diagnostics turned on. The single feature of this code referreded in other parts of the application is a signature of the method taking request string_view and bool diagnostics as parameters and returning the response string. Different logic from a different field, not necessarily finance, can be plugged in. 

In order to measure performance of the system the same batch was repeated in an infinite loop. I was mostly interested in the server performance, so requests from the client were compressed once (optional, "PrepareBatchOnce" in ProgramOptions.json on the client side) and then repeatedly sent to the server. The server was processing these batches from scratch in each iteration. With one client processing of one batch takes 26 to 30 milliseconds on a rather weak laptop, the client command being './client > output.txt' or even './client > /dev/null'. Printing to the terminal doubles the latency.

To test the code:

1. clone the repository

2. build the app

boost libraries must be installed.

google tests must be installed:\
sudo apt-get install libgtest-dev\
sudo apt-get install cmake\
cd /usr/src/gtest\
sudo cmake CMakeLists.txt\
sudo make\
sudo cp lib/*.a /usr/lib

The compiler must support c++20\
I use\
gcc  11.1.0 and\
clang 12.0.0\
boost_1_78_0

make -j3

g++ is used by default. To use clang add CMPLR=clang++ to the make command.\
There are options for sanitizers (address, undefined, leak or thread), profiler, different optimization levels.\
see the makefile.

3. run\
server:

Change settings for FIFO and tcp in ProgramOptions.json:

server:\
fifo:\
  "FifoDirectoryName" : "directory anywhere with appropriate permissions",\
  "FifoBaseNames" : "Client1 Client2 Client3 Client4 Client5"

tcp:
  "TcpPort" : "49152",
  "Timeout" : 5

client #1:\
communication type:\
  "CommunicationType" : ["TCP" | "FIFO"]

fifo:\
  "FifoDirectoryName" : "the same as for the server",\
  "FifoBaseName" : "Client1",

tcp:\
  "ServerHost" : "server hostname",\
  "TcpPort" : "49152"

  ........

Names of pipes and tcp ports should match on server and clients. ProgramOptions.json contains the\
list for 5 clients, but this number can be increased, only performance can put the\
limit. The server was tested with 5 clients with mixed client types.

FIFO files are created on server startup and removed on server shutdown with Ctrl-C.\
By design, the clients should not create or remove FIFO files. The code does not do\
this, besides fifo directory permissions do not allow this.\
........

To start:\
on server side './server' in the project root\
on client side './client' in the client_bin or any other directory.

No special requirements for the hardware.\
Using laptop with 4 cores and 4GB RAM to run the server and up to 5 clients.\
Running Linux Mint 20.2.

Multiple runtime options are in ProgramOptions.json files for the server and client
in corresponding directories.

Some of these:

"StringTypeInTask" : [STRINGVIEW | STRING]\
Algorithm selection. STRINGVIEW is slightly faster but with different memory allocation pattern.

"ProcessRequestMethod" : ["Transaction", "Echo"]\
'Transaction' is a business logic.\
'Echo' allows to test multithreading, compression, and fifo/tcp. To check the results save client\
output to the file and diff this file with the source (default is 'requests.log').

To enable compression (default)\
"Compression" : "LZ4"\
If this is empty or something different the compression is disabled.

"Timing" prints elapsed times between selected lines of the code. Currently it shows client latency\
for the batch of 10000 requests and the total run time for the server. See Chronometer class in the code.

"DYNAMIC_BUFFER_SIZE" is the size of the memory pool entries.\
This parameter controls the task size(the number of requests in the batch) and\
memory footprint of the application. The latter is important for embedded systems.

'"Diagnostics" : true' shows details of all stages of business calculations for each request.\
Alternative to this global setting is '"Diagnostics" : true' in the client ProgramOptions.json which enables diagnostics only for that client. Server restart is not necessary in this case.

To run tests:\
'tests/runtests'\
in the project root directory.

=======
### Fast Lockless Linux Client-Server with TCP and FIFO clients
Using both bidirectional named pipes and tcp.\
Lockless. Processing batches of requests  without locking.\
Business logic, tasks multithreading, and communication layer are completely decoupled.\
Memory pooling. Business logic, compression and most of fifo processing are not allocating.\
Business logic here is an example of financial calculations.\
It can be replaced with any other batch processing from a different field, not necessarily financial.
