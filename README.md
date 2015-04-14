MALT : Malloc Tracker
=====================

What it is ?
------------

MALT is a memory tool to find where you allocate your memory. It also provide you some
statistics about memory usage and help to find memory leaks.

Dependencies
------------

MALT depend on presence of :

 - binutils (nm and add2line) to extract symbols. Tested version is 2.24.

It optionaly depend on :

 - nodejs (http://nodejs.org/) to run the GUI webserver. Tested version is 0.10.30.
 - libelf (http://www.mr511.de/software/english.html) to extract global variable list from executables and libs. Tested version is 0.128.
 - libunwind (http://www.nongnu.org/libunwind/) as alternative implementation of glibc backtrace method. Tested version is 1.1.

How to install
--------------

MALT use CMake for the build system but provide a simple configure wrapper for users
familiar with autotools packaging so you can install by following the procedure :

	mkdir build
	cd build
	../configure --prefix={YOUR_PREFIX}
	make
	make test
	make install

If you want more advance usage you need to call cmake by yourself so you can install it 
by following the procedure :

	mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX={YOUR_PREFIX}
	make
	make test
	make install

Build options
-------------

MALT build support several options to define with -D option of CMake :

 * -DENABLE_CODE_TIMING={yes|no} : Enable quick and dirty function to measure MALT internal
   performances.
 * -DENABLE_TEST={yes|no}        : Enable build of unit tests.
 * -DJUNIT_OUTPUT={yes|no}       : Enable generation of junit files for jenkins integration.
 * -DENABLE_VALGRIND={yes|no}    : Run unit tests inside valgrind memcheck and generate XML report.
 * -DPORTABILITY_OS={UNIX}       : Set portability build option to fix OS specific calls.
 * -DPORTABILITY_MUTEX={PTHREAD} : Set portability build option to select mutex implementation.

How to use
----------

MALT currently provide a dynamic library you need to preload in your application to
wrap the default memory allocator. It provide two basic instrumentation modes.

By default MALT use backtrace to reconstruct you stack on malloc/free/... calls :

	{YOUR_PREFIX}/bin/malt {YOUR_PROGRAM} [OPTIONS]

You can get better performance but less detailed stack by using option 
-finstrument-function or similar on your compiler. Then, you need to tel MALT to use
the "enter-exit" stack mode :

	{YOUR_PREFIX}/bin/malt -m=enter-exit {YOUR_PROGRAM} [OPTIONS]

The malt script only provide a wrapper to automatically preload a dynamic library
into the executable, you can also do it by hand in cas of issue :

	LD_PRELOAD={YOUR_PREFIX}/lib/libmalt.so {YOUR_PROGRAM} [OPTIONS]

Using webview
-------------

You can use the webview by calling command `malt-webview` as :

	malt-webview [-p PORT] -i malt-YOUR_PROGRAM-1234.json

It will open a server listening localy on port 8080 so you can open your web browser
to connect to the web interface via http://localhost:8080.

If you are running the view remotly thought SSH you can redirect the ports by using :

	ssh -L 8080:localhost:8080 user@ssh-server

To use the webview you need to install the nodeJS package on your system : http://nodejs.org/.

Config
------

You can provide a config file to MALT to setup some features. This file use the INI
format. With the malt script :

	{YOUR_PREFIX}/bin/malt -c=config.ini" {YOUR_PROGRAM} [OPTIONS]

By hand :

	MALT_CONFIG="config.ini" LD_PRELOAD=libmalt.so {YOUR_PROGRAM} [OPTIONS]

Example of config file :

	[time]
	enabled=true          ; enable time profiles
	points=1000           ; keep 1000 points
	linar_index=false     ; use action ID instead of time

	[stack]
	enabled=true          ; enable stack profiles
	mode=backtrace        ; select stack tracing mode (backtrace|enter-exit)
	resolve=true          ; Automatically resolve symbol with addr2line at exit.

	[output]
	name=malt-%1-%2.%3    ; base name for output, %1 = exe, %2 = PID, %3 = extension
	indent=true           ; indent the output
	lua=true              ; enable LUA output
	json=true             ; enable json output
	callgrind=true        ; enable callgrind output
	config=true           ; dump current config

Option values can be override on the fly with command :

	{YOUR_PREFIX}/bin/malt -o "stack:enabled=true;output:indent=true;" {YOUR_PROGRAM} [OPTIONS]

Environnement variables
-----------------------

If you do not use the malt wrapper and use directly LD_PRELOAD you can use the Environnement variables :

	MALT_OPTIONS="stack:enabled=true;output:indent=true;"
	MALT_CONFIG="config.ini"
	MALT_STACK="libunwind"

About stacks
------------

MALT use two ways to rebuild stacks, the default one rely on glibc backtrace but we observe severals 
segfaults on some intel tools such as Intel OpenMP and Intel MPI so we also provide a more robust 
approach based on libunwind if present on your system at build time. You can provide it with :

	../configure --with-libunwind=PREFIX
	
or on cmake :

	cmake -DLIBUNWIND_PREFIX=PREFIX ..

You now can use it with malt by using :

	malt -s libunwind {PROGRAM}

The alternative rely on function instrumentation by adding prove on start/end for each function.
It can be done by using -finstrument-function on your compiler juste as described in "How to use" section
or by using binary instrumentation tools juste as explained at the end of this document.

Tracking stack size
-------------------

Malt can also track the memorty used by stacks over time, but for this support it is required to
enable a compiler flag :

	gcc -finstrument-functions {YOUR FILES}

Experimental pintool mode
-------------------------

MALT can also use binary instrumentation mode through pintool 
(http://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool)

Please, check usage into src/pintool directory.

Experimental maqao mode
-----------------------

MALT can also use binary instrumentation with MAQAO (http://maqao.org/). 

Please check usage into src/maqao directory.
