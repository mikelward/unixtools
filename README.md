unixtools
=========

An excuse to write some C code for Linux-like systems.

l
-

`l` is my take on `ls`.  Intended to compatible with GNU, BSD, and POSIX ls
where it makes sense, but add some options to make it more useful to me.

### New features

`l -D` displays only directories.

`l -L` follows and displays symlink information, e.g.

    $ l -L link_to_link_to_file
    link_to_link_to_file -> link_to_file -> file

with appropriate colors (`-G` or `-K`) or flags (`-F` or `-O`) if requested.

`-M` displays "modes" (i.e. permissions) for the _current_ user, hopefully
making it more useful than the `-rwxr-xr-x user group` format of `ls -l`,
e.g.

    $ ls -M file_i_can_only_read
    r-- file_i_can_only_read

(this may be renamed to `-p` to mean "permissions".)

The intent is to give each part of the `ls -l` output a separate flag letter,
so you can pick and choose which fields you want to display, e.g. `ls -ogsT`
should display the owner, group, size, modification time, and name of each file.
After they are all implemented, adding `-l` will be easy.

###Supported options

####File selection
 * directories only (`-D`)
 * show all (including hidden files) (`-a`)

#### File properties
 * append a flag showing the file's type (`-F`)
 * append a flag showing the file's type - old BSD style (`-O`)
 * show symlink target information (`-L`)
 * show size field (`-s`)
 * show inode number field (`-i`)

#### Display format
 * columns (`-C`)
 * rows (`-x`)
 * one-per-line (`-1`)

#### Sorting
 * sort by name (default)
 * sort by mtime (modification time, `-t`)
 * reverse sort (`-r`)
 * don't sort (`-f` or `-U`)

#### Coming soon
 * show date and time (maybe `-T`?)
 * show file modes field (maybe `-m`?)
 * sort by ctime (change time, `-c`)
 * sort by btime (creation time, a.k.a. birth time, `-b`, or maybe `-U`)
 * print question marks instead of control characters (`-q`)
 * escape control characters (maybe `-e`?)

#### Coming later
 * show/toggle file owner field (`-o` or `-O`)
 * show/toggle file group field (`-g` or maybe `-G`)
 * long listing (`-l` = modes+owner+group+size+date+time+name)
   (not sure whether to do block count, major/minor numbers, etc.)
 * show user and group ids instead of names (`-n`)
 * other stuff

#### To investigate
 * remove `-f` option (same as `-U`)
 * `-I <pattern>` to ignore files matching `<pattern>`
 * file ACLs and extended attributes
 * list major/minor numbers for block/character devices
 * customizable colors
 * symlink options, including `-H`, `-L`, `-P`, and interaction with `-f`, `-F`, etc.
 * sort by atime (access time, `-u`), not sure how useful this is?

#### Incompatibilities
 * GNU Emacs Dired mode (`-D`)
 * `-L` shows more information than classic `ls -L`
 * escape mode (`-b`), assuming birth time option is implemented as `-b`
 * legacy `-g` (long without owner) option, depending on how `-g` is implemented
 * stream mode (`-m`), assuming file modes `-m` option is implemented
 * legacy `-o` (long without group) option, depending on how `-o` is implemented
 * append slash to directories (`-p`), assuming `-M` becomes `-p`
   need to double check `-F` symlink behavior before doing this

Patches are welcome.
