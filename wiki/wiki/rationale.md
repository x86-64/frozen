# Rationale

* **Flow-based programming**

There is a lot of frameworks and different api, and all of them require lot of learning and code writing to describe what you really want to do. Despite of 
incredible flexibility, this approach is very annoying. Most of use cases reduce to several standard variations, for which there is no examples at all.
Let's look at electronic engineering - it have simple parts, like transistors, ICs, microcontrollers, and circuit was done by hands. But look for state of art
in this field - it is model oriented design. Engineer describe process to perform in diagrams and software generate code to run on microprocessor. With this approach
it is easy to make changes, maintain and test things. Why not use this in general programming?
So, today frozen can look silly - most of modules repeat coreutils and common tools. But tomorrow you could look at big flat screen with huge block diagram
which in realtime show load to every computer in your cluster and you could change any parameter on fly, add some more workers and so on.

* **Configuration driven software**

Nowadays there is too much of software in "install-and-forget" style. This kind of software is useful in most cases only for
first time use, but then you start using it turns out it was pre-configured in source code to achieve some goals, which in turn
can not match with your's. Only you can know how to construct most optimal solution of your problem. Fancy software can't do this,
For example, one ip address can be address of server in nearby rack, and other ip can be on the another side of earth, so what software could do?
Measure latency of every ip it see? It will result in horrible, complex code with lot of bugs and overhead to each request. But if where was
the way to tell "hey, this link would be slow, let's set up a buffer or even compress data. But rest of links will keep send plain data".
No way your current database can do so.

* **Configure every aspect**

Big configs filled with comments to every "max_recv_buffer_size" and alike drives people crazy. There is no image of how the hell this thing working,
and which parameter belong to which part of software. And this lead to bad configuration and resource wasting.

* **Distributed and fault-tolerant systems**

To build such system need to know all possible solutions, pick one, learn it, sharpen your skills with it and finally use it in your
project. And every time you face changes - you need to rewrite code and test it again. We need a new system, with which there would be
no reason to write any code related to distribution part and no reason to rewrite existing modules. Flow-based programming allow us to do
that. You write modules with clean interface. You write simple configuration files and use existing macro sets for distribution, load-balancing
and fault-tolerance. If anything happens - you only change some configuration and whole system work again like a charm.
