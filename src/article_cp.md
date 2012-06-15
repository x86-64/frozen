Title: Flow-based programming intro
Date: 17-02-2012

### Copying files in simple way

What could be simplier than to copy file? May be this is not very interesting way to spend your time, but lets take a look
how we can do job using frozen and it's flow-based nature. First, assume we want to copy file within same computer.
We need to open both files, read some from one file to buffer, write this buffer to second file, and in both steps monitor return codes,
right? No. We just call "transfer" method on file, like so:

[article_cp_01_simple.m4][]

	$ frozend -c examples/article_cp_01_simple.m4 -o "-DINPUT=TODO -DOUTPUT=TODO.bak"

That was pretty easy. But that if we want to copy file from different machines?

### Copying file to another computer

If we want to copy something to another machine usually we call scp or rsync, but i always wanted to do this using ZeroMQ socket.
So, let's do it! We need to open file, open zmq socket, read some info from file and then send message, then repeat until file ends, right?
Again, no, and again, we use "transfer". Out (client) side configuration file will look like so:

[article_cp_02_client.m4][]

Not much changes for completely different destination. FILE macro hides some magic, it define new machine with given name and zeromq port.
This machine pass all requests to remote side for zeromq socket and get reply from it. Transfer routine of file didn't know there actual
write goes, so it could be anywhere, even on another machine.

Let's take a look at server side, maybe it is very complex?

[article_cp_02_server.m4][]

Not at all. It have endless thread which read from zeromq socket, then EXPLODE request from incoming message and pass it to file.
To use this we run in shell following commands:

	$ frozend -c examples/article_cp_02_server.m4 -o "-DOUTPUT=TODO.bak" &
	$ frozend -c examples/article_cp_02_client.m4 -o "-DINPUT=TODO"

### Copying files between computers

Next step is to copy between two computers. Client:

[article_cp_03_client.m4][]

Pretty obvious changes. We replace file to special machine and everything works just fine. Server side is same, just some macro added.
To see it in action:

	frozend -c examples/article_cp_03_server.m4 -o "-DFILE=TODO -DPORT=8888" &
	frozend -c examples/article_cp_03_server.m4 -o "-DFILE=TODO.bak -DPORT=8887" &
	frozend -c examples/article_cp_03_client.m4 

However, data from file flows through our (client) computer and this is not so good. Possible solutions:
* Pass zeromq socket as destination itself. Yes we can transfer socket object from one computer to another and everything would work fine, but unfortunately "transfer" method not well suitable for this. This is because socket is waiting for clean data and transfer will pass clean data, but other side server is expecting not clean data, but request. So, we need to do special server side - not best solution.
* Make "transfer" daemon. It would be standalone daemon, which would listen to our requests and will initiate transfer. Data would flow through daemon. Still not best.
* Make "proxy" pattern for zeromq socket. In this pattern socket act as proxy, passing real requests to it to another end. This will not require server-side changes. Cool, but it not yet implemented, but would be.

### Enumerating things on LevelDB

Copying files led us to nice idea - why bother to get data to ourselves, let's shoot everything with it. Last nice example would be
remote LevelDB instance using our favorite ZeroMQ sockets.

[example_leveldb_zmq.m4][]

As you can see, to enumerate database items we pass socket. And we can point this socket to any point in your network. But to see than it is actually
working we point to ourselves and get all data. Notice how our (incoming) socket used in mustache parser. It use real object to enumerate it and
zeromq support it, returning all messages as items with key "data".

[article_cp_01_simple.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_cp_01_simple.m4
[article_cp_02_client.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_cp_02_client.m4
[article_cp_02_server.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_cp_02_server.m4
[article_cp_03_client.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_cp_03_client.m4
[article_cp_03_server.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_cp_03_server.m4
[example_leveldb_zmq.m4]: https://github.com/x86-64/frozen/blob/master/examples/example_leveldb_zmq.m4

