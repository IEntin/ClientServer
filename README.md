Copyright (C) 2021 Ilya Entin.

### Fast Linux Lockless Client-Server with FIFO and TCP clients, compression and encryption

To test this software clone the project, run './makeCrypto.sh <Crypto++ zip file name>'.\
This script will download requested source and build the library in /usr/local directory,\
one has to be a superuser to do that,current source is cryptopp880.zip.\
Then run deploy.sh script in the project root.\
Run './makeCrypto.sh -h' and './deploy.sh -h' to see the details.\
Crypto file .cryptoKey.sec is created by the server and must be distributed\
securely to the clients after server startup but\
before clients are started. Use copyCryptoKey.sh to distribute the key\
to clients after server startup. Optionally key is generated every time the server\
starts and clients need to use a new key every time. By default however, the key is\
valid until erased (the setting "InvalidateKey" is false in ServerOptions.json).\
Test scripts refresh keys internally. Note that crypto initialization vector iv\
is generated anew for every message being encrypted and sent for further processing\
inside the message.

Prerequisites:

Header only boost libraries, currently boost 1_82.

google tests must be installed:\
sudo apt-get install libgtest-dev\
sudo apt-get install cmake\
cd /usr/src/gtest\
sudo cmake CMakeLists.txt\
sudo make\
sudo cp lib/*.a /usr/lib

The compiler must support c++20\
clang, currently 14.0.0\
and/or\
gcc, currenrly 11.3.0\
some previous versions will do as well.

This server works with multiple mixed tcp and fifo clients.

Tcp communication layer is using boost Asio library. Every session is running in its own thread\
(io_context per session). This approach has its advantages and disadvantages. There is an\
overhead of context switching but connections are independent hence more reliable, the logic\
is simpler and allows new features like waiting mode, see below.

Session can be extremely short-lived, e.g. it might service one submillisecond request or\
in another extreme it can run for the life time of the server. With this architecture it\
is important to limit creation of new threads and use thread pools. 

This server is using thread pools for both tcp and fifo sessions, see ThreadPool... classes in\
the code. Thread pool creates threads on demand comparing the number of objects of a given\
class and the number of already created threads. This prevents creation of redundant threads\
possible if maximum allowed number of threads were created in advance.

In some cases fifo has better performance and is more secure than tcp. Due to protocol\
with predictable sequence of reads and writes, it is possible to make pipes bidirectional. This\
situation is unlike e.g. chat application or similar when unidirectional fifo is normally used.\
In this application after every write the code closes write file descriptor and opens read fd for the\
same end of the pipe, and so on. This significantly simplifies the setup - one fifo per client. See\
fragments of the configuration in ServerOptions.json for the server and ClientOptions.json for clients.\
Testing showed that performance impact of fd reopening is small.
........

The number and identity of fifo and tcp clients are not known in advance.\
Tcp clients are using the standard ephemeral port mechanism.\
For fifo clients an analogous acceptor method is used.\
The fifo acceptor is a special pipe waiting for a request for a new session\
from the starting client on a blocking fd read open(...) call. The client\
unblocks acceptor by opening its writing end. The server (acceptor) generates\
UUID pipe name, creates the pipe and a new session, and sends pipe name to\
the client.\
Only one starting client creates a session at a time. This process is\
syncronized by a named_mutex. Tests show that any number of clients\
can be started concurrently by the script checkmulticlients.sh.\
Globally unique pipe name is an analogy of the unique combination\
of ip address and ephemeral port in the tcp case.

.........

There are configurable restrictions on the number of tcp and/or fifo sessions the server\
is handling. If a new client exceeds this limit it remains up but enrers a waiting mode.\
When one of the previously started clients exits, the waiting client at the head of the\
queue starts running.\
Another restriction is on the total number of sessions.\
Both restrictions can be applied simultaneously. The second takes precedence.
There can be any number of waiting clients.\
A waiting client can also be closed and tried again later.\
The relevant settings are "MaxTcpSessions", "MaxFifoSessions" and "MaxTotalSessions"\
in ServerOptions.json.

There are no software or hardware restrictions on the number of clients. Observed\
latency is strictly proportional to the number of clients due to increasing load\
on worker threads running processing logic. Only performance is a limiting factor.\
This server was tested with 20 clients of mixed types. The server and all clients are\
running on the same desktop. In real case tcp clients will run on different hosts.

........

Fifo code is increasing the pipe size to make read and write "atomic".\
This operation requires sudo access if requested size exceeds 1MB\
(with original configuration, buffer size is 64 KB). If more than 1MB is necessary:\
sudo ./server\
and\
sudo ./client\
in appropriate directories.\
The same for scripts profile.sh and runtests.sh.\
If you do not have sudo access, binaries and scripts will still run for any\
requested size but buffer size will not be changed and setPipeSize(...) will\
log an error "Operation not permitted".\
This option can be disabled altogether by setting\
"SetPipeSize", false in ServerOptions.json and ClientOptions.json.

If fifo is not an option the client can switch to the tcp mode and reconnect to the server without\
stopping the server.

Lockless. Processing batches of requests is lockless. Queues of tasks and sessions are\
still using locks, but this type of locking is relatively rare for large tasks and does not\
affect overall performance.

Memory reuse. Business code, compression, and encryption are not allocating after short\
adjustment period.

Builtin optional LZ4 compression.

Server allows multi phase request processing. The preprocessor phase in the current code\
is generation of the specific key and sorting requests by this key.

Business logic, compression, task multithreading, and communication layers are decoupled.

Business logic is an example of financial calculations I once worked on. This logic finds keywords in the request from another document and performs financial calculations based on the results of this search. There are 10000 requests in a batch, each of these requests is compared with 1000 entries from another document containing keywords, money amounts and other information. The easy way to understand this logic is to look at the responses with diagnostics turned on. The single feature of this code referred in other parts of the application is a signature of the method taking request string_view as a parameter and returning the response string. Different logic from a different field, not necessarily finance, can be plugged in.

To measure the performance of the system the same batch is repeated in an infinite loop, but every time it is created anew from a source file. The server is processing these batches from scratch in each iteration. With one client processing one batch takes about 10 milliseconds on desktop with 4 CPU cores and on Ubuntu 22.04. Printing to the terminal doubles the latency, use ./client > /dev/null or write output to the file to exclude/reduce printing latency.

To test the code manually (not using deploy.sh):

1. clone the repository

2. build the app

make arguments:

SANITIZE = [ aul | thread |  ] By default disabled.\
  aul stands for address,undefined,leak. aul and thread sanitizers\
  cannot be used together.\
PROFILE = [ 1 | 0 |  ] By default disabled.\
ENABLEPCH = [ 1 | 0 |  ] By default enabled.\
  add 'ENABLEPCH=0' to the make command to disable PCH.\
OPTIMIZE = [ -O3 | any |  ] By default -O3.\
CMPLR = [ clang++ | g++ |  ] By default clang++.

There are four targets:\
server client testbin runtests.\
runtests is a pseudo target which invokes running\
tests if this target is older than test binary testbin.\
By default all targets are built:\
'make -j4'\
(4 is the number of cpu cores)\
Any combination of targets can be specified, e.g.\
'make -j4 server client'

3. run\
server:

Change settings for FIFO and tcp in ServerOptions.json:

server:\
fifo:\
  "FifoDirectoryName" : "directory anywhere with appropriate permissions",\
  "AcceptorName" : "Acceptor", named pipe for fifo acceptor.

tcp:
  "TcpPort" : "49152",\
  "Timeout" : 5,

client #1:\
communication type:\
  "ClientType" : ["TCP" | "FIFO"]

fifo:\
  "FifoDirectoryName" : "the same as for the server",\
  "AcceptorName" : "Acceptor", the same as for the server

tcp:\
  "ServerHost" : "server hostname",\
  "TcpPort" : "49152"

To start:\
in the server terminal\
cd project_root\
./server

in another client terminal\
cd project_root\
./client

To run the server outside of the project create a directory, copy server binary,\
make a soft link to the project_root/data directory, copy ServerOptions.json\
to this directory, and issue './server' command.

Similarly for the client: create a directory, copy binary client, make a soft link to data\
directory, copy ClientOptions.json, and run './client'.

No special requirements for the hardware.\
Using laptop with 4 cores and 4GB RAM to run the server and up to 5 fifo and tcp clients.\
Running Linux Mint 20.2.\
Multiple runtime options are in ServerOptions.json and ClientOptions.json files\
in corresponding directories.

Some of these:

LogThreshold controls the amount of logging selecting one of 5 levels
TRACE, DEBUG, INFO, WARN, and ERROR, e.g. with setting
"LogThreshold" : "INFO"
log statements with level INFO and higher will be printed.

Compression\
"Compression" : "LZ4"\
If the value is empty or different the compression is disabled.

"Timing" prints elapsed times between selected lines of the code. Currently it shows client latency\
for the batch of 10000 requests and the total run time for the server. See Chronometer class in the code.

"DYNAMIC_BUFFER_SIZE" is the size of the memory pool entries.\
This parameter controls the task size(the number of requests in the batch) and\
memory footprint of the application. The latter is important for embedded systems.

"NumberTaskThreads" is the number of work threads. The default\
value 0 means that the number of threads is std::hardware_concurrency

Client can request diagnostics for a specific task to show details of all stages of business calculations.\
This setting is '"Diagnostics" : true' in the ClientOptions.json. It enables diagnostics only\
for that client.

To run Google tests:\
'./testbin'\
or './runtests.sh <number repetitions>' \
in the project root. './runtests.sh -h' for help.

To run the tests from any directory outside of the project\
create a directory anywhere, make a soft link to the\
project_root/data directory in this new directory and copy the binary\
testbin from the project_root, issue './testbin' command.\
Note that testbin is not using program options. Instead, it is using\
defaults.

Script profile.sh runs profiling of server and both client types.\
 The usage is\
'./profile.sh' in the project root. Two directories for clients\
should exist with necessary files and links.

Script checkstuff.sh runs builds and tests application with both\
g++ and clang++, with and without sanitizers. Run './checkstuff -h' to see usage.

We run memory and thread sanitizer and performance profiling for every commit.\
Warnings are considered failures.

=======
### Fast Lockless Linux Client-Server with TCP and FIFO clients, compression and encryption

Using both bidirectional named pipes and tcp.\
Lockless: except infrequent queue operations.\
Processing batches of requests  without locking.\
Optimized for cache friendliness.\
Business logic, tasks multithreading, and communication layer are decoupled.

Memory pooling. Most of processing in stable regime after startup is not allocating.\
Tuning the memory pool size allows to drastically reduce memory footprint of the software,\
especially of the client, with moderate speed decrease, which can in turn reduce hardware\
requirements.

Business logic is an example of financial calculations. It can be replaced with any other\
batch processing from a different field, not necessarily financial.
