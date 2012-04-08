unixtools
=========

An excuse to write some C code for Linux-like systems.

l
-

`l` is my take on `ls`.  Intended to compatible with GNU, BSD, and POSIX ls
where it makes sense, but add some options to make it more useful to me.

Examples:

`l -D` displays only directories.

`l -L` follows and displays symlink information, e.g.

    $ l -L link_to_link_to_file
    link_to_link_to_file -> link_to_file -> file

with appropriate colors (-G or -K) or flags (-F or -O) if requested.

`-M` displays "modes" (i.e. permissions) for the _current_ user, hopefully
making it more useful than the `-rwxr-xr-x user group` format of `ls -l`,
e.g.

    $ ls -M file_i_can_only_read
    r-- file_i_can_only_read

(this may be renamed to `-p` or `-P` to mean "permissions".)

The intent is to give each part of the `ls -l` output a separate flag letter,
so you can pick and choose which fields you want to display, e.g. `ls -ogsT`
should display the owner, group, size, modification time, and name of each file.

Column (`-C`), row (`-x`) and one-file-per-line (`-1`) options are supported,
as are some sort options.  (More to come.)  `-l` is also being worked on.

I haven't yet decided whether to support uncommon flags such as `-m`.  I think
maybe those letters could be reassigned to make `l` more useful for interactive
use.  (e.g. for `blocksize = 1 megabyte` or to display the mode field without
requiring use of `-l`).

Patches are welcome.
