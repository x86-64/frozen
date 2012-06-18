# Rationale

* **Make it simple**

There is a lot of frameworks and different api, and all of them require lot of learning and code writing to describe what you really want to do. Despite of 
incredible flexibility, this approach is very annoying. Most of use cases reduce to several standard variations, for which there is no examples at all.
Let's look at electronic engineering - it have simple parts, like transistors, ICs, microcontrollers, and circuit was done by hands. But look for state of art
in this field - it is model oriented design. Engineer describe process to perform in diagrams and software generate code to run on microprocessor. With this approach
it is easy to make changes, maintain and test things. Why not use this in general programming?

* **Make it clean**

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

