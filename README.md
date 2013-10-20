Concurrency Control Flow 并发流程控制
========

A C++ framework is based on coroutine and libuv, just using **start**, **await**, **all_of**, **any_of** to control flow.

一个基于协程和libuv的C++框架，仅通过 **start**、 **await**、 **all_of**、 **any_of** 控制流程。

### Documents: [wiki](https://github.com/chishaxie/cocoflow/wiki)

### To build:

Prerequisites (Linux):

    * GCC 4.2 or newer
    * Python 2.6 or 2.7
    * GNU Make 3.81 or newer
    * (All of above is for libuv)
	
Linux:

    make
	
Prerequisites (Windows):

    * UnZip (You can extract dependencies by other tools)
    * Python 2.6 or 2.7
    * Visual Studio 2008 or 2010 or 2012
	
Windows:

    run vcbuild.bat
	
### To run the tests:

Linux:

    make test
	
Windows:

	run vc\test.bat
	
### License

(The LGPL License)
