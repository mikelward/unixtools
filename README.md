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

`-p` displays permissions for the _current_ user, hopefully
making it more useful than the `-rwxr-xr-x user group` format of `ls -l`,
e.g.

    $ l -M file_i_can_only_read
    r-- file_i_can_only_read

`l -O` prints directory names inside square brackets and executables inside
asterisks, e.g.

    $ l -M
    [dir1]  [dir2]  *exe*   file

###Supported options

####File selection
 * directories only (`-D`)
 * show all (including hidden files) (`-a`)
 * list subdirectories recursively (`-R`)

#### File properties
 * show inode number field (`-i`)
 * show size in blocks (`-s`)
 * show file modes (`-M` or `-m`), e.g. `-rwxr-xr-x.`
 * show link count (`-N`)
 * show file owner (`-o`)
 * show file group (`-g`)
 * show size in bytes (`-B` or `-b`)
 * show file modification time (`-T`)
 * show file change time (`-Tc`)
 * show file access time (`-Tu`)
 * append a flag showing the file's type (`-F`) _makes `-H` default to off_
 * append a flag showing the file's type - old BSD style (`-O`)
 * long format (`-l`, same as `-MNogBT1`) _makes `-H` default to off_
 * show numeric owner and group instead of looking up their names (`-n`)
 * show time in ISO 8601 format (`-I`), e.g. `2012-05-30 20:30:40`
 * show file symlink chain (`-V`), e.g. `link1 -> link2 -> file`

#### Symlinks
 * follow symlinks (show information about symlink target, `-L`)
 * don't follow symlinks (show information about symlink itself, `-P`)
 * follow symlinks to directories specified as command line arguments (`-H`, defaults to on unless `-P`, _`-F`, or `-l`_ were given)

 If `-H` is not specified:

  * if `-L` is specified, `-H` is enabled; if `-P` is specified, `-H` is disabled
  * _else if `-F` or `-l` are specified, `-H` is disabled_
  * otherwise, `-H` is enabled

#### Display format
 * columns (`-C`)
 * rows (`-x`)
 * one-per-line (`-1`)

 _`-C` is the default if output is a terminal, otherwise `-1`._

#### Sorting
 * sort by name (default)
 * sort by size (`-S`)
 * sort by mtime (modification time, `-t`)
 * sort by ctime (change time, `-tc`) _just `-c` is sufficient if neither `-T` nor `-l` were given_
 * sort by atime (access time, `-tu`) _just `-u` is sufficient if neither `-T` nor `-l` were given_
 * sort by version (numeric order, `-v`)
 * reverse sort (`-r`)
 * don't sort (`-f` or `-U`)

#### Escaping
 * print control characters as question marks (`-q`)
 * print control characters using C-style escapes (`-e`)
 * disable escaping (`-E`)

_`-q` is the default if output is a terminal, otherwise `-E`._

#### Coming soon
 * sort by btime (creation time, a.k.a. birth time, `-b`, or maybe `-U`)
 * show file ACLs (`-A`?)
 * human-readable file sizes (`-h`?)
 * file sizes in megabytes and gigabytes (`-M`, `-G`?)

#### Coming later
 * locale and Unicode support
 * other stuff

#### To investigate
 * remove `-f` option (same as `-U`)
 * make `-e` the default instead of `-q`?
 * `-I <pattern>` to ignore files matching `<pattern>`
 * file ACLs and extended attributes
 * list major/minor numbers for block/character devices
 * customizable colors
 * colors and flags for files with setuid/setgid/sticky bits or capabilities
 * sub-second times for sorting and display
 * SELinux (and other security systems) support (e.g. `.` in modes, `-Z` flag)
 * escape all fields, e.g. usernames, etc.
 * tabular output format, e.g. `<field>[\t<field>]*\n` (no need for null separation given -e flag)

#### Incompatibilities
 * `-D` - lists only directories, rather than GNU Emacs Dired mode
 * `-b` - adds a file size in bytes column (use `-e` to escape file names)
 * `-g` - adds a group column, rather than useless long-without-owner
 * `-m` - adds a modes column, rather than stream mode
 * `-o` - adds an owner column, rather than long-without-group (could rename to `-O` if `-o` is really needed)
 * `-p` - adds a permissions column, rather than appending slash to directories (use `-F` or `-O`)
 * `-H` - only follows links to directories, similar to GNU ls rather than BSD ls
 * `-f` - only disables sorting, similar to BSD ls rather than GNU ls

### Build

You need a C99 compiler, GNU make, and `ncurses` and `libacl` development packages.

Debian/Ubuntu: `sudo apt-get install gcc make libncurses-dev libacl1-dev`.  
Red Hat/Fedora: `sudo yum install gcc make ncurses-devel libacl-devel`.

Then, run `make`.

### Installation

Run `make install`.  Files are installed under `/usr/local` by default.  Install
somewhere else by doing `make install DESTDIR=<path>`.

Patches and pull requests are welcome.
