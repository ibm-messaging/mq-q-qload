Building the programs

Included in this package is a Makefile for Linux x64.
You can use that Makefile as a template for other platforms, 
with appropriate #define values. Often the platform compiler
will define some of the values that can be seen as #ifdef 
elements in this source tree, but some MQ-related definitions
may also be needed.

Used #defines in the code which differentiate platforms include

AMQ_AS400
AMQ_UNIX (for all Unix and Linux platforms)
MVS
WIN32, WIN64,  WINDOWS, _WINDOWS
HP_NSK

Within the Unix/Linux family, there are also
subdivisions for the #defines. You will see
_AIX
_HPUX_SOURCE
_LINUX_2
_SOLARIS_2



