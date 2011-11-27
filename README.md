Frozen
=============

Introduction
-------------

Frozen is data processing daemon driven by configuration files. It contain several modules for input, output and data processing.
It can be used to construct your own database, simple web server, data collector, logger and so on.

Architecture of frozen based on Unix philosophy of simple parts and clean interfaces. This allows usage of existing modules in various places.
If some functionality not present, you can write it and connect it to frozen. There is no limit on language to use - if it can compile to shared
library - it is very likely to work properly.

Capabilities
-------------

* Fuse filesystem for input and output
* Storage in memory or file
* Memory caches
* Indexes, lookup tables
* Pack/unpack from defined structure
* Ipc
* Load balancing
* Benchmarks


Rationale
--------

* Model-oriented programming

 There is a lot of frameworks and different api, and all of them require lot of learning and code writing to decribe what you really want to do. Despite of
 incredible flexibility, this approach is very annoying. Most of use cases reduce to several standard variations, for which there is no examples at all.
 Let's look at electronic engineering - it have simple parts, like transistors, ICs, microcontrollers, and circuit was done by hands. But look for state of art
 in this field - it is model oriented design. Engineer describe process to perform in diagrams and software generate code to run on microprocessor. With this approach
 it is easy to make changes, maintain and test things. Why not use this in general programming?
 So, today frozen can look silly - most of modules repeat coreutils and common tools. But tomorrow you could look at big flat screen with huge block diagram
 which in realtime show load to every computer in your cluster and you could change any parameter on fly, add some more workers and so on.

* Configuration driven software
 
 Nowadays there is too much of software in "install-and-forget" style. This kind of software is useful in most cases only for
 first time use, but then you start using it turns out it was pre-configured in source code to achieve some goals, which in turn
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
 
Requirements
-----------
 * check (for tests)
 * fuse (for fuse filesystem)
 * flex, bison, perl (if you make clean and want to rebuild from scratch, optional)
 * gccgo (for Go modules)


Links
-----
 * [Project page](http://x86-64.github.com/frozen/)
 * [Tutorials](http://x86-64.github.com/frozen/html/group__tutorial.html)
 * [Documentation](http://x86-64.github.com/frozen/html/)

