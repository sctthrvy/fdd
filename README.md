fdd
====
####File Descriptor Daemon

A daemon to create and pass descriptors to processes.

The main motivation of fdd is allowing an under privileged processes to obtain
raw and packet sockets, however it may also support some other dangerous tasks
(such as opening arbitrary files).

Only processes running as root (or with the CAP_NET_RAW capability) may create
a raw or packet socket, but once a socket is created any process that obtains
the socket descriptor can use it.

fdd could be run as root, however we recommend that on Linux it rather be run
with the CAP_NET_RAW capability (using setcap(8) and getcap(8)).
