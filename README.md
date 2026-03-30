unixtools
=========

An excuse to write some C code for Linux and macOS (and other Unix-like systems).

l
-

`l` is my take on `ls`.  Intended to compatible with GNU, BSD, and POSIX ls
where it makes sense, but add some options to make it more useful to me.

### New features

`l -D` displays only directories.

`l -V` follows and displays symlink information, e.g.

    $ l -V link_to_link_to_file
    link_to_link_to_file -> link_to_file -> file

with appropriate colors (`-G` or `-K`) or flags (`-F` or `-O`) if requested.

`-p` displays permissions for the _current_ user, hopefully
making it more useful than the `-rwxr-xr-x user group` format of `ls -l`,
e.g.

    $ l -p file_i_can_only_read
    r-- file_i_can_only_read

`l -O` prints directory names inside square brackets and executables inside
asterisks, e.g.

    $ l -O
    [dir1]  [dir2]  *exe*   file

`l -h` displays human-readable file sizes using SI units (1000-based), e.g.
`1 KB`, `23 MB`.

`l --time-style=relative` displays file times as relative durations, e.g.
`3 days`, `2 hours`, `45 minutes`.

### Supported options

#### File selection
 * directories only (`-D`, `--dirs-only`)
 * show all (including hidden files) (`-a`, `--all`)
 * list directory names instead of their contents (`-d`, `--directory`)
 * list subdirectories recursively (`-R`, `--recursive`)

#### File properties
 * show inode number field (`-i`, `--inode`)
 * show size in blocks (`-s`, `--size`)
 * show file modes, e.g. `-rwxr-xr-x.` (`-M` or `-m`, `--modes`)
 * show link count (`-N`, `--link-count`)
 * show file owner (`-o`, `--owner`)
 * show file group (`-g`, `--group`)
 * show size in bytes (`-B` or `-b`, `--bytes`)
 * show file modification time (`-T`, `--show-time`)
 * show file change time (`-Tc`)
 * show file access time (`-Tu`)
 * show device major/minor numbers for block and character devices (in place of size)
 * append a flag showing the file's type (`-F`, `--classify`) _makes `-H` default to off_
 * append a flag showing the file's type - old BSD style (`-O`, `--old-flags`)
 * long format (`-l`, `--long`, same as `-MNogBT1`) _makes `-H` default to off_
 * show numeric owner and group instead of looking up their names (`-n`, `--numeric-uid-gid`)
 * show time in ISO 8601 format (`-I`, `--iso`), e.g. `2012-05-30 20:30:40`
 * show file symlink chain (`-V`, `--show-links`), e.g. `link1 -> link2 -> file`
 * human-readable file sizes (`-h`, `--human-readable`), e.g. `1 KB`, `23 MB`

#### Symlinks
 * follow symlinks (show information about symlink target, `-L`, `--dereference`)
 * don't follow symlinks (show information about symlink itself, `-P`, `--no-dereference`)
 * follow symlinks to directories specified as command line arguments (`-H`, `--dereference-command-line`, defaults to on unless `-P`, _`-F`, or `-l`_ were given)

 If `-H` is not specified:

  * if `-L` is specified, `-H` is enabled; if `-P` is specified, `-H` is disabled
  * _else if `-F` or `-l` are specified, `-H` is disabled_
  * otherwise, `-H` is enabled

#### Display format
 * columns (`-C`, `--columns`)
 * rows (`-x`, `--rows`)
 * one-per-line (`-1`, `--one-per-line`)

 _`-C` is the default if output is a terminal, otherwise `-1`._

#### Colors
 * `-G`, `-K`, `--color` enables colored output

 Colors are obtained via terminfo. Assignments: blue for directories, cyan for
 symlinks, green for executables, red for files that cannot be stat'd.

#### Sorting
 * sort by name (default)
 * sort by name (`--sort=name`, default)
 * sort by size (`-S`, `--sort=size`, largest first)
 * sort by mtime (modification time, `-t`, `--sort=time`, newest first)
 * sort by ctime (change time, `-tc`, `--ctime`, `--time=ctime`) _just `-c` is sufficient if neither `-T` nor `-l` were given_
 * sort by atime (access time, `-tu`, `--atime`, `--time=atime`) _just `-u` is sufficient if neither `-T` nor `-l` were given_
 * sort by btime (birth/creation time, `--btime`, `--time=btime`) _uses Linux statx() syscall_
 * sort by mtime (modification time, `--mtime`, `--time=mtime`) _this is the default_
 * sort by version (numeric order, `-v`, `--sort=version`)
 * reverse sort (`-r`, `--reverse`)
 * don't sort (`-f`, `-U`, `--unsorted`, `--sort=none`)

 When sorting by time or size, ties are broken by name (alphabetical).
 Files that cannot be stat'd sort last.

#### Escaping
 * print control characters as question marks (`-q`, `--hide-control-chars`)
 * print control characters using C-style escapes (`-e`, `--escape`)
 * disable escaping (`-E`, `--no-escape`)
 * Unicode-aware: correct column alignment for wide (CJK) and combining characters

_`-q` is the default if output is a terminal, otherwise `-E`._

#### Time display
 * ISO 8601 format (`-I`, `--iso`): `YYYY-MM-DD HH:MM:SS`
 * traditional format (default): day+time for recent files, month+year for older files
 * relative format (`--time-style=relative`): e.g. `3 days`, `2 hours`

### Display layout

Column display (`-C`) fills entries down columns first (newspaper style),
row display (`-x`) fills across rows first.

Columns are separated by a 2-space outer margin. Fields within a line are
separated by a 1-space margin. Fields are right- or left-aligned and padded to
the maximum width across all entries for consistent column layout.

Terminal width is detected via `ioctl(TIOCGWINSZ)` on stdout, defaulting to 80
columns if detection fails or output is not a terminal.

### ACL support

In long format (`-l` or `-M`), the mode string includes an 11th character:
`+` if extended ACLs are present, or ` ` (space) if not.  ACL detection is
skipped for symlinks and on filesystems that don't support ACLs.

### Directory listing behavior

When given multiple arguments or using `-R`, directory headers are printed as
`dirname:` with blank lines between sections. Files listed on the command line
are always displayed before directories.

With `-s` or `-l`, a `total <blocks>` line is printed before each directory's
contents, showing the sum of block allocations. The default block size is 1024
and can be overridden with the `BLOCKSIZE` environment variable.

### Error handling

Errors are printed to stderr. Most errors (stat failures, permission errors,
etc.) are non-fatal - the program continues processing remaining files.
Invalid options cause the program to print usage and exit with code 2.

### Coming soon
 * show file ACLs (`-A`?)
 * file sizes in megabytes and gigabytes (`-M`, `-G`?)

### Coming later
 * other stuff

### To investigate
 * remove `-f` option (same as `-U`)
 * make `-e` the default instead of `-q`?
 * `-I <pattern>` to ignore files matching `<pattern>`
 * file ACLs and extended attributes
 * customizable colors
 * colors and flags for files with setuid/setgid/sticky bits or capabilities
 * sub-second times for sorting and display
 * SELinux (and other security systems) support (e.g. `.` in modes, `-Z` flag)
 * escape all fields, e.g. usernames, etc.
 * tabular output format, e.g. `<field>[\t<field>]*\n` (no need for null separation given -e flag)

### Incompatibilities
 * `-A` - not implemented
 * `-b` - adds a file size in bytes column (use `-e` to escape file names)
 * `-D` - lists only directories, rather than GNU Emacs Dired mode
 * `-g` - adds a group column, rather than useless long-without-owner
 * `-m` - adds a modes column, rather than stream mode
 * `-o` - adds an owner column, rather than long-without-group (could rename to `-O` if `-o` is really needed)
 * `-p` - adds a permissions column, rather than appending slash to directories (use `-F` or `-O`)
 * `-H` - only follows links to directories, similar to GNU ls rather than BSD ls
 * `-f` - only disables sorting, similar to BSD ls rather than GNU ls

### Build

You need a C99 compiler, GNU make, and `ncurses` development packages.
ACL support also requires `libacl` on Linux.

Debian/Ubuntu: `sudo apt-get install gcc make libncurses-dev libacl1-dev`.
Red Hat/Fedora: `sudo yum install gcc make ncurses-devel libacl-devel`.
macOS: `xcode-select --install` (provides compiler and make; ncurses is included).

Then, run `make`.

### Installation

Run `make install`.  Files are installed under `/usr/local` by default.  Install
somewhere else by doing `make install DESTDIR=<path>`.

Patches and pull requests are welcome.
