Frozen
=============

Introduction
-------------

Frozen is data processing daemon driven by configuration files. It contain several modules for input and output as well as data processing.
It can be used to construct your own database, simple web server, data collector, logger and so on.

Architecture of frozen based on Unix philosophy of simple parts and clean interfaces. This allows usage of existing modules in various places.
If some functionality not present, you can write it and connect it to frozen. There is no limit on language to use - if it can compile to shared
library - it is very likely to work properly.

Capabilities
-------------

* Fuse filesystem for input and ouput
* Storage in memory or file
* Memory caches
* Indexes, lookup tables
* Pack/unpack from defined structure
* Ipc
* Load balancing
* Benchmarks


Rational
--------

* Configuration driven software
 
 Nowdays there is too much of software in "install-and-forget" style. This kind of software is useful in most cases only for
 first time use, but then you start using it turns out it was pre-configured in source code to achive some goals, which in turn
 can not match with your's. Only you can know how to construct most optimal solution of your problem. Fancy software can't do this,
 For example, one ip address can be address of server in nearby rack, and other ip can be on the another side of earth, so what software could do?
 Measure latency of every ip it see? It will result in horrible, complex code with lot of bugs and overhead to each request. But if where was
 the way to tell "hey, this link would be slow, let's set up a buffer or even compress data. But rest of links will keep send plain data".
 No way your current database can do so.
     
* Configure every aspect
  Big configs filled with comments to every "max_recv_buffer_size" and alike drives people crazy. There is no image of how the hell this thing working,
  and which parameter belong to which part of software. And this lead to bad configuration and resource wasting.
         
* Reduce communication within single machine
 
  To setup a web server we need to install many different components: database, server, may be some cache, balancer, fastcgi handlers and so on. And every part
  must be connected somehow. But any connection introduce significant latency and limit performance of overall system. So, if there is only one physical machine, why
  bother with it? Lets join all components to one process. Simple call within process is best avaliable to world communication channel, it is fast and simple.
 
Requirments
-----------
 * check (for tests)
 * fuse (for fuse filesystem)
 * flex, bison, perl (if you make clean and want to rebuild from scratch, optional)
 * gccgo (for Go modules)


