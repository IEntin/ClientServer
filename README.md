Copyright (C) 2021 Ilya Entin.

### Fast Linux Lockless Clent-Server with FIFO and TCP clients

This server can work with multiple mixed tcp and fifo clients.

Tcp communication layer is using boost Asio library. Every connection is running in its own thread\
(io_context per connection). Connections can be extremely short-lived, e.g. it might service one\
submillisecond request or in another extreme it can run for the life time of the server. With this\
architecture it is important to avoid creating new threads and use thread pools.

This server is using thread pools for both tcp and fifo connections, see ThreadPool class for a\
generic thread pool.

In some cases fifo has better performance and possibly stronger security than tcp. Due to protocol\
with predictable sequence of reads and writes, it is possible to make pipes bidirectional. This\
situation is unlike e.g. chat application or similar when unidirectional fifo is normally used.\
After every write the code closes write file descriptor and opens read fd for the same end of the\
pipe, and so on. This significantly simplifies the setup - one fifo per client. See below fragments\
of configuration in ProgramOptions.json for the server and clients. Testing showed that performance\
impact of fd reopening is small for large batches.

If fifo is not an option the client can switch to the tcp mode and reconnect to the server without\
stopping the server.

Lockless. Processing batches of requests is lockless. Queues of tasks and connections are\
still using locks, but this type of locking is relatively rare for large tasks and does not\
affect overall performance.

Memory Pooling. Business code and compression/decompression are not allocating after startup.

Builtin optional LZ4 compression.

Business logic, compression, task multithreading, and communication layers are completely decoupled.

Business logic here is an example of financial calculations I once worked on. This logic finds keywords in the request from another document and performs financial calculations based on the results of this search. There are 10000 requests in a batch, each of these requests is compared with 1000 entries from another document containing keywords, money amounts and other information. The easiest way to understand this logic is to look at the responses with diagnostics turned on. The single feature of this code referreded in other parts of the application is a signature of the method taking request string_view as a parameter and returning the response string. Different logic from a different field, not necessarily finance, can be plugged in.

In order to measure performance of the system the same batch was repeated in an infinite loop, but every time it was created anew from a file. The server was processing these batches from scratch in each iteration. With one client processing of one batch takes about 30 milliseconds on a rather weak laptop, the client command being './client > output.txt' or even './client > /dev/null'. Printing to the terminal doubles the latency.

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

clang++ compiler is used by default. To use g++ add CMPLR=g++ to the make command.\
There are options for sanitizers (address, undefined, leak or thread), profiler, different optimization levels,\
see the makefile.

3. run\
server:

Change settings for FIFO and tcp in ProgramOptions.json:

server:\
fifo:\
  "FifoDirectoryName" : "directory anywhere with appropriate permissions",\
  "FifoBaseNames" : "Client1 Client2 Client3 Client4 Client5"

tcp:
  "TcpPort" : "49152",\
  "Timeout" : 5,\
  "ExpectedTcpConnections" : 5 // this must be equal or greater than the number of tcp clients

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

Names of pipes and tcp ports should match on server and clients. For named pipes ProgramOptions.json\
contains the list of known clients. The number and identity of tcp clients is not known in advance, only\
their expected maximum number("ExpectedTcpConnections" in ProgramOptions.json).\
Generally, the number of clients is limited by hardware performance.\
This server was tested with 5 clients with mixed client types.

FIFO files are created on server startup and removed on server shutdown with Ctrl-C.\
By design, the clients should not create or remove FIFO files. The code does not do\
this, besides fifo directory permissions do not allow this.\
........

To start:\
on server side './server' in the project root\
on client side './client' in the client_bin or any other directory.

No special requirements for the hardware.\
Using laptop with 4 cores and 4GB RAM to run the server and up to 5 fifo and tcp clients.\
Running Linux Mint 20.2.\
Multiple runtime options are in ProgramOptions.json files for the server and client\
in corresponding directories.

Some of these:

Compression\
"Compression" : "LZ4"\
If this is empty or different the compression is disabled.

"Timing" prints elapsed times between selected lines of the code. Currently it shows client latency\
for the batch of 10000 requests and the total run time for the server. See Chronometer class in the code.

"DYNAMIC_BUFFER_SIZE" is the size of the memory pool entries.\
This parameter controls the task size(the number of requests in the batch) and\
memory footprint of the application. The latter is important for embedded systems.

"NumberTaskThreads" is the number of work threads. The default\
value 0 means that the number of threads is std::hardware_concurrency

Client can request diagnostics for a specific task to show details of all stages of business calculations.\
This setting is '"Diagnostics" : true' in the client ProgramOptions.json. It enables diagnostics only\
for that client.

To run the Google tests:\
'./runtests'\
in the tests directory\
or './runtests.sh'\
in the project root.

It is also possible to run tests from any directory if it is outside of the\
project. To do this create a directory anywhere, make a soft link to the\
project root/data directory in this new directory and copy the binary\
runtests from the project root/tests. Then issue './runtests'.

Script profile.sh runs automatic profiling of the server and clients.\
 The usage is\
'./profile.sh'\
in the project root.

The conventional procedure in this project includes thread and memory sanitizer runs and performance\
profiling of every commit. Sanitizer warnings are considered failures and are not accepted.

=======
### Fast Lockless Linux Client-Server with TCP and FIFO clients
Using both bidirectional named pipes and tcp.\
Lockless. Processing batches of requests  without locking.\
Business logic, tasks multithreading, and communication layer are completely decoupled.\
Memory pooling. Business logic, compression and most of fifo processing are not allocating.\
Business logic here is an example of financial calculations.\
It can be replaced with any other batch processing from a different field, not necessarily financial.
