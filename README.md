mq-q-qload
==========

This repository contains the code from IBM MQ SupportPacs MA01 (q) and MO03 (qload)


Abstract
========
This repository contains 
* A simple pipe line program (Q) which takes messages from one source and outputs to a target.
* QLoad provides a simple command line application for loading and unloading messages off queues. 

Description
===========

Q takes messages from one source and outputs to a target. The operation of the pipe is controlled by switches to the program. The source and target can either be the console, a Queue or a Topic.

Possible Uses of Q
* To browse the formatted contents of a queue.
* To move/copy messages from one queue to another.
* To load a set of test messages from a file.
* To generate a set of simple test messages, for example to put 25 10K messages 5 seconds apart.
* To act as a queue splitter. Take all messages off one queue and re-put to two different queues.

Possible Uses of QLoad
* To save the messages that are on a queue, to a file, perhaps for archiving purposes and the possibility of later reload back onto a queue.
* To reload a queue with messages you previously saved to a file.
* To remove messages from a queue which are old.
* To ‘replay’ test messages from a stored location – even maintaining time spacing if required.

The Queue Load / Unload Utility allows the user to copy or move the contents of a queue, its messages, to a file. This file can be saved away as required and used at some later point to reload the messages back onto the queue. This file has a specific format understood by the utility, but is human-readable, so that it can be updated in an editor before being reloaded.


Please note: A version of qload is now (from MQ v8) part of the product - renamed to dmpmqmsg - and as such is fully supported through PMRs/APARs.


History
=======

MA01 (q)
--------
First Released: 29 Aug 1995

Last Updated: 17 May 2012

MO03 (qload)
------------
First Released: 11 Apr 2005

Last updated: 12 Jul 2012


Contributions to this package can be accepted under the terms of the 
IBM Contributor License Agreement, found in the file CLA.md of this repository.