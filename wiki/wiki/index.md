# Frozen

## Introduction
  Frozen is [flow-based][] data processing daemon. It contain several modules for input, output and data processing.
  It can be used to construct complex data processing systems in short time, reuse existing solutions and code, connect "not-connectable" things.

  There is one very nice approach called "Big Data" - it present new techniques for data processing allowing to scale in any amounts. Another one
  approach is industrial assembly lines - old but very good way of constructing things. Combining both of them we can achieve incredible levels
  of scalability and, in same time, simplicity. However, to do real stuff we need tools too, so frozen tries to fill gap in this field.

  Architecture of frozen based on Unix philosophy of simple parts and clean interfaces. This allows usage of existing modules in various places.
  If some functionality not present, you can write it and connect it to frozen. There is no limit on language to use - if it can compile to shared
  library - it is very likely to work properly. With ZeroMQ module you can reuse any existing code, despite of nature of language.

## Goals
  * Create flexible tool to manage huge amounts of data
  * Make tool to build distributed and fault-tolerant systems in easy way
  * Standardize data storage and transmission formats
  * Get rid of temporary scripts, conversion utilities, all sort of one-time parsers

## Capabilities
  * Common data types: ints, strings, lists, hashes and more  
  * Embedded packing and unpacking routines for any type of data
  * Processing modules: regular expressions, switches and many more
  * Web development related modules: mongrel2, mustache, leveldb
  * IO modules: fuse, file, memory, sockets
  * Parallelization and distributed systems on top of ZeroMQ network
  * Zero-copy everywhere it possible
  * M4 macro in configuration files
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

