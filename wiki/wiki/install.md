# How to install

## Get archive
 
To get archive you can use any of the following methods:

* <a href="https://github.com/x86-64/frozen/zipball/master">Download as .zip</a>
* <a href="https://github.com/x86-64/frozen/tarball/master">Download as .tar.gz</a>
* git clone https://github.com/x86-64/frozen.git
* git clone git://github.com/x86-64/frozen.git

## Compile

Compilation as easy as any other package.

	$ ./configure
	$ make
	$ make check
	$ make install

"make check" is optional, but recommended.

Configuration script automatically find all installed software, such as ZeroMQ library and use it. If not - this
modules would be disabled and installation continue.

