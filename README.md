# nodetool-cleanup
Small program using zookeeper to coordinate scylla and cassandra nodetool cleanups across hosts in a cluster. Assumes you already have a zookeeper server. Example of how to run it from cron: 

38 3 * * sun,wed root command -p time -o /tmp/nodetool_compact.out nodetool compact
30 23 1 * * root /usr/local/sbin/run_nodetool_repair zk_server:2181 scylla_cluster_name > /tmp/run_nodetool_repair.out 2>&1

To compile:

as root on a scylla host:
wget https://archive.apache.org/dist/zookeeper/zookeeper-3.4.5/zookeeper-3.4.5.tar.gz
unpack it into /usr/local/src
cd /usr/local/src/zookeeper-3.4.5/src/c
./configure; make; make install

as yourself:
export LD_LIBRARY_PATH=/usr/local/lib
export ZK_HOME=/usr/local/src/zookeeper-3.4.5/
gcc -Wall run_nodetool_repair.c -o run_nodetool_repair -I${ZK_HOME}/src/c/include -I${ZK_HOME}/src/c/generated/ -L/usr/l
ocal/lib/  -lzookeeper_mt

Zookeeper API:

There are no man pages. Two sources of information about the API:
- https://zookeeper.apache.org/doc/r3.4.6/zookeeperProgrammers.pdf
- the source code, particularly c/include/zookeeper.h

A watch is a one-time thing. Whenever a watch is triggered, you must
explicitly set a new watch in order to be informed of further events.

Certain calls (zookeeper_init, zoo_get) set watches. 
The function that is assigned as part of the call, 
will automatically run when the watch is triggered. This is is handled 
by zookeeper; there is no need to procedurally call the function. 
Therefore, in most cases, you will want, inside the function, to make 
a call that sets another watch and assigns itelf as the function,
although you can give it a different function if you need.

Things like zoo_get default to the function assigned to zookeeper_init.
This is what this program does.

With things like zoo_wget, you can assign a different function to be
called when an event is triggered. This function has its own watcher
context (different the global one associated with zookeeper_init).

zookeeper_init automatically sets a watch for session events, and
the function given in its parameter list gets called when this event
is triggered. 

--

The run_nodetool_cleanup program automatically runs a thread for the watcher function, and 
another thread for the builtin callback functionality. These threads
keep running as long as the main program runs, therefore we use 
pause() to ensure the main program runs continuously.

Certain api calls (eg. zoo_aset) can be done either asynchronously
or synchronously. This program makes use of synchcronous calls only.
(Except for zookeeper_init which is always asynchronous).
If you choose to use asynchronous calls, then you must invoke completion
routines to check their status.

A full list of the api calls is in /usr/local/include/zookeeper/zookeeper.h.
