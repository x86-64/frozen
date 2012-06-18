# Frozen

## Introduction
  Frozen is data processing and management library with additional cli tool. It can be used to construct complex data storage solutions,
  distributed processing pipelines in short time.

  Architecture of frozen based on Unix philosophy of simple parts and clean interfaces. This allows usage of existing modules in various places.
  Frozen introduce simple and powerful set of api which all internal modules should conform. This approach guarantee that any module will work
  with any other module without any problems.

  Frozen have a lot of functionality built in, but if this is not enough you can easily add your own in any language you want. This is possible
  because of ZeroMQ module, which have huge support from all other languages.

## Goals of this project
  * Create flexible tool to manage huge amounts of data
  * Make tool to build distributed and fault-tolerant systems in easy way
  * Use data in it original form, without importing or exporting

## Capabilities
  * Can construct NoSQL storage on top of existing data
  * Can construct distributed systems, queues, workers
  * Web development related modules: mongrel2, mustache, leveldb
  * IO modules: fuse, file, memory, sockets
  * Zero-copy everywhere it possible
  * Benchmarking and measurements

## Get started

* [Rationale][rationale]
* [Tutorial][tutorial]
* [Examples][examples]
* [How to install][install]

## Get involved

* [Blog][blog]
* [Github project page][github]
* [Doxygen files][doxygen]
* [Discussion group & announcements][group]

[flow-based]: http://en.wikipedia.org/wiki/Flow-based_programming
[install]: install.html
[rationale]: rationale.html
[tutorial]: tutorial_basics.html
[examples]: https://github.com/x86-64/frozen/tree/master/examples
[blog]: http://x86-64.github.com/frozen/blog/
[doxygen]: http://x86-64.github.com/frozen/doxygen/
[group]: http://groups.google.com/group/frozend
[github]: https://github.com/x86-64/frozen

