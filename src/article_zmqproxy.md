Title: Remote data access with ZeroMQ proxies
Date: 05-12-2012

Now with ZeroMQ data proxy we can access remote data which hosted in another frozen instance in same way
as it could be inside same process.
It looks like:

[data_zeromq_proxy_t.fz][]

Here is "client" variable represent remote data. And "server" is data on remote server. For testing purpose it was created
in same process, however communications go only through ZeroMQ socket.
Data we access is simple hash, with "testkey" which store test value.

[data_zeromq_proxy_t.fz]: https://github.com/x86-64/frozen/blob/master/examples/data_zeromq_proxy_t.fz

