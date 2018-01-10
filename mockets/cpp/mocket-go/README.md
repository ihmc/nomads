This Golang package is a partial wrapper of the IHMC Mockets.
Using the wrapper is possible to use DTLS to enable secure connection over Tactical Networks.
The Send and Receive methods have a 1:1 mapping with the C++ code. It is possible 
to create custom implementations of the send and receive with parameters such as reability, sequence, priority etc..

Supported Platforms:
Linux x86_64
Windows //TO-DO VS2015 project to build the wrapper

How to compile the wrapper? (Currently only Linux is supported)
make
The make execution produces a shared library, libmocketgowrapper.so which is used by the Go code.
The package also contains the serverTest and clientTest directory with examples of how to use the mocket-go wrapper.

Filippo Poltronieri <fpoltronieri@ihmc.us>


