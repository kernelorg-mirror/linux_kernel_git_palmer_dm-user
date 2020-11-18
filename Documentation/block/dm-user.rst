.. SPDX-License-Identifier: GPL-2.0

=======
dm-user
=======

What is dm-user?
================

dm-user is a device mapper target that allows block accesses to be satisfied by
an otherwise unprivileged daemon running in userspace.  Conceptually it is
FUSE, but for block devices as opposed to file systems.

Creating a dm-user Target
=========================

dm-user is implemented as a Device Mapper target, which allows for various
device management tasks.  In general dm-user targets function in the same
fashion as other device-mapper targets, with the exception that dm-user targets
handle requests via a userspace daemon as opposed to one of various in-kernel
mechanisms.  As such there is little difference between creating a dm-user
target and any other device mapper target: the standard device mapper control
device and ioctl() calls are used to create a table with at least one target of
the "user" type.  Like all other targets this table entry needs a start/size
pair.  The additional required argument is the name of the control device that
will be associated with this target.  Specifically:

````
user <start sector> <number of sectors> <path to control device>
````

As a concrete example, the following `dmsetup` invocation will create a new
device mapper block device available at `/dev/mapper/blk`, consisting entirely
of a single target which can be controlled via a stream of messages passed over
`/dev/dm-user/ctl`.

````
dmsetup create blk <<EOF
0 1024 user 0 1024 ctl
EOF
dmsetup resume blk
````

Userspace is expected to spin up a daemon to handle those block requests, which
will block in the meantime.  The various Device Mapper operations should all
function as they do for other targets.  If the userspace daemon terminates (or
otherwise closes the control file descriptor) then the kernel will continue to
queue up BIOs until userspace either starts a new daemon or destroys the
target.

Userspace may open each control device multiple times, in which case the kernel
will distribute messages among each file instance.

Writing a dm-user Daemon
========================

tools/testing/selftests/dm-user contains a handful of test daemons.
functional/simple-read-all.c is also suitable as an example.

Kernel - userspace interface
****************************

FIXME: A description of `struct dm_user_message`

Kernel Implementation
=====================

BIO Lifecycle
*************

| "dd if=/dev/mapper/user ...             " | dm-user block server
|                                           |
|                                           | >sys_read()
|                                           |  >dev_read()
|                                           |   [sleep on c->wq]
|                                           |
| >sys_read()                               |
|  [... block and DM layer ... ]            |
|  >user_map()                              |
|   [enqueue message]                       |
|   [wake up c->wq]                         |
|  <user_map()                              |   [woken up]
| [sleep on BIO completion]                 |   [copy message to user]
|                                           |  <dev_read()
|                                           | <sys_read()
|                                           |
|                                           | [obtain request data]
|                                           |
|                                           | >sys_write()
|                                           |  >dev_write()
|                                           |   [copy message from user]
|                                           |   [complete BIO]
|  [woken up on BIO completion]             |  <dev_write()
| <sys_read()                               | <sys_write()
|                                           |
| [write and loop]                          | [loop for more messages]

Locking Scheme
**************
