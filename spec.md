# `l` - Specification

This document specifies the behavior of the `l` file listing utility (a custom `ls` replacement). It is derived from the existing C implementation.

## Overview

`l` is a file listing utility compatible with GNU, BSD, and POSIX `ls` where sensible, with additional custom features. It lists files and directories with configurable sorting, formatting, coloring, and metadata display.

## Command-Line Interface

```
Usage: l [-1aBbCcDdEeFfGgHhIiKkLlMmNnOoPpqRrSsTtUuVvx] [OPTION]... [<file>]...
```

If no file arguments are given, list the current directory (`.`).

## Options

### File Selection

| Flag | Long option | Description |
|------|-------------|-------------|
| `-a` | `--all` | Show all files, including hidden files (names starting with `.`) |
| `-D` | `--dirs-only` | Show only directories (filter out non-directories) |
| `-d` | `--directory` | List directory names themselves, not their contents |
| `-R` | `--recursive` | Recursively list subdirectories |

### Metadata Fields

Fields are displayed in the following order when enabled (left to right):

1. **Size in blocks** (`-s`, `--size`) - right-aligned; also enables directory totals line
2. **Inode** (`-i`, `--inode`) - right-aligned
3. **Modes** (`-M` or `-m`, `--modes`) - left-aligned, 11-character string (see [Mode String Format](#mode-string-format))
4. **Link count** (`-N`, `--link-count`) - right-aligned
5. **Owner** (`-o`, `--owner`) - left-aligned (name by default, numeric with `-n`)
6. **Group** (`-g`, `--group`) - left-aligned (name by default, numeric with `-n`)
7. **Permissions** (`-p`, `--perms`) - right-aligned, 3-character string showing current user's effective permissions via `access()`: `r`/`-`/`?`, `w`/`-`/`?`, `x`/`-`/`?`
8. **Size in bytes** (`-B` or `-b`, `--bytes`) - right-aligned
9. **Date/time** (`-T`, `--show-time`) - right-aligned (see [Date/Time Format](#datetime-format))
10. **Name** - left-aligned in columns/rows mode, unpadded in one-per-line mode

### Long Format

`-l` (`--long`) is equivalent to `-MNogBT1` and also enables:
- `showlink`: display symlink target (one level: `link -> target`)
- `dirtotals`: print `total <blocks>` line before each directory listing

### Numeric IDs

`-n` (`--numeric-uid-gid`) shows numeric uid/gid instead of looking up user/group names.

### File Type Indicators

| Flag | Long option | Style | Before name | After name |
|------|-------------|-------|-------------|------------|
| `-F` | `--classify` | POSIX | (none) | `/` dir, `@` link, `\|` FIFO, `=` socket, `*` exec, ` ` regular |
| `-O` | `--old-flags` | BSD   | `[` dir, `@` link, `*` exec, `?` unstat | `]` dir, `@` link, `*` exec, `?` unstat, ` ` regular |

For `-F`: files that cannot be stat'd get `?` after the name.

### Symlink Display

| Flag | Long option | Description |
|------|-------------|-------------|
| `-V` | `--show-links` | Show full symlink chain: `link1 -> link2 -> file` (with colors/flags on each component). Detects loops via inode tracking. |
| (with `-l`) | | Show immediate target only: `link -> target` |

### Symlink Following

| Flag | Long option | Description |
|------|-------------|-------------|
| `-L` | `--dereference` | Follow symlinks: show information about target instead of link itself. Implies `-H`. Disables `showlink`. |
| `-P` | `--no-dereference` | Don't follow symlinks: show information about the link. Implies no `-H`. |
| `-H` | `--dereference-command-line` | Follow symlinks to directories specified as command-line arguments. |

**`-H` default logic** (when not explicitly given):
1. If `-L` was given: `-H` is ON
2. If `-P` was given: `-H` is OFF
3. Else if `-F` or `-l` was given (compatible mode): `-H` is OFF
4. Otherwise: `-H` is ON (default)

When `-L` is active, if the final target of a symlink cannot be determined, fields display `?` values.

### Display Format

| Flag | Long option | Description |
|------|-------------|-------------|
| `-C` | `--columns`, `--format=vertical` | Display in columns (fill down first, then across). Default when stdout is a terminal. |
| `-x` | `--rows`, `--format=across` | Display in rows (fill across first, then down) |
| `-1` | `--one-per-line`, `--format=single-column` | One entry per line. Default when stdout is not a terminal. |

`--format=long` is equivalent to `-l` (`--long`).

### Sorting

| Flag | Long option | Description |
|------|-------------|-------------|
| (default) | `--sort=name` | Sort alphabetically by name using `strcoll()` (locale-aware) |
| `-S` | `--sort=size` | Sort by file size (`st_size`), largest first |
| `-t` | `--sort=time` | Sort by modification time, newest first |
| `-v` | `--sort=version` | Sort by version using `strverscmp()` (numeric-aware) |
| `-r` | `--reverse` | Reverse the sort order |
| `-f` or `-U` | `--unsorted`, `--sort=none` | Don't sort (directory order). Also disables `-r`. |

#### Time type modifiers

| Flag | Long option | Effect |
|------|-------------|--------|
| `-c` | `--time=ctime` | Use ctime (status change time) for sorting and display |
| `-u` | `--time=atime` | Use atime (access time) for sorting and display |

**Compatibility behavior**: If `-c` or `-u` is given without `-T` or `-l`, it implies `-t` (sort by time). This matches traditional `ls` behavior where `-c` alone means "sort by ctime".

#### Sort tie-breaking

All time and size sorts fall back to alphabetical name comparison (`strcoll()`) when values are equal.

#### Stat failure in sorting

Files that cannot be stat'd sort as if they have the smallest value (they appear last in normal order, first when reversed).

### Escaping Non-Printable Characters

| Flag | Long option | Mode | Description |
|------|-------------|------|-------------|
| `-q` | `--hide-control-chars` | ESCAPE_QUESTION | Replace control characters with `?`. Default when stdout is a terminal. |
| `-e` | `--escape` | ESCAPE_C | C-style escapes: `\n`, `\t`, `\a`, `\b`, `\v`, `\f`, `\r`, `\\`, or `\NNN` for others |
| `-E` | `--no-escape` | ESCAPE_NONE | No escaping; output raw bytes. Default when stdout is not a terminal. |

Escaping applies to file names only. The `\\` literal backslash is doubled only in ESCAPE_C mode. Wide character / multibyte handling via `mbrtowc()` + `iswprint()` + `iswcntrl()`.

### Color

| Flag | Long option | Description |
|------|-------------|-------------|
| `-G` | `--color` | Enable color (FreeBSD compatibility) |
| `-K` | `--color` | Enable color (mnemonic: "kolor") |

**Color assignments** (via terminfo `setaf`):
- Red: files that cannot be stat'd
- Blue: directories
- Cyan: symlinks
- Green: executables
- Default (no color): regular files

Color reset uses terminfo `sgr0`. Colors are obtained via `setupterm()` + `tigetstr("setaf")` + `tparm()`. If the terminal does not support colors, color is silently disabled.

Color escape sequences contribute zero display width (important for column alignment).

### Size Display

| Flag | Long option | Description |
|------|-------------|-------------|
| `-h` | `--human-readable` | Human-readable sizes (e.g., `1 KB`, `23 MB`). Uses SI units (1000-based). |
| `-k` | `--kibibytes` | Block size of 1024 (default is also 1024) |

The `BLOCKSIZE` environment variable sets the default block size if present.

Human-readable format: `%.0f UNIT` where units are B, KB, MB, GB, TB, PB, EB, ZB, YB. Threshold for next unit is 1000.

### Date/Time Format

| Flag | Long option | Description |
|------|-------------|-------------|
| `-I` | `--iso` | ISO 8601 format: `YYYY-MM-DD HH:MM:SS` |
| | `--time-style=traditional` | Traditional format (default) |
| | `--time-style=relative` | Relative time (e.g., `3 days`, `2 hours`) |

**Traditional format** (the default) uses three tiers:
1. Less than 6 days old: `%a    %H:%M` (e.g., `Mon    14:30`)
2. Less than 180 days old: `%b %e %H:%M` (e.g., `Jan  5 14:30`)
3. Older or in the future: `%b %e  %Y` (e.g., `Jan  5  2023`)

**Relative format** uses: `N years`, `N months`, `N days`, `N hours`, `N minutes`, `N seconds`. Falls back to traditional for future timestamps.

Time types: modification time (default), change time (`-c`/`-Tc`), access time (`-u`/`-Tu`).

## Display Layout

### Column Layout (`-C`)

Items fill down columns first (newspaper style):
```
0  2  4
1  3  5
```

### Row Layout (`-x`)

Items fill across rows first:
```
0  1  2
3  4  5
```

### Layout Calculations

- **Outer margin**: 2 spaces between columns
- **Field margin**: 1 space between fields within a row
- **Column width**: `max_entry_width + 2` (outer margin)
- **Max columns**: `screen_width / column_width` (minimum 1)
- **Screen width**: from `ioctl(TIOCGWINSZ)` on stdout, fallback to 80

For column mode (`-C`):
- `rows = ceil(count / max_cols)`
- `cols = ceil(count / rows)`
- Index mapping: `idx = col * rows + row`

For row mode (`-x`):
- Sequential index, newline after every `cols` items

### Field Alignment and Padding

Each field has a maximum width computed across all entries. Fields are padded to this width:
- `ALIGN_RIGHT`: spaces before content
- `ALIGN_LEFT`: spaces after content
- `ALIGN_NONE`: no padding (used for name field in one-per-line mode)

Fields are separated by 1 space (the column margin). The last field in a row has no trailing spaces.

## Mode String Format

11-character string: `tRWXrwxrwx+`

Position 1 - file type:
- `l` symlink, `d` directory, `b` block device, `c` char device, `p` FIFO, `s` socket, `-` regular

Positions 2-4 - user permissions:
- `r`/`-`, `w`/`-`, `x`/`-` (or `s`/`S` if setuid)

Positions 5-7 - group permissions:
- `r`/`-`, `w`/`-`, `x`/`-` (or `s`/`S` if setgid)

Positions 8-10 - other permissions:
- `r`/`-`, `w`/`-`, `x`/`-` (or `t`/`T` if sticky)

Position 11 - ACL indicator:
- `+` if extended ACLs present (checked via `acl_get_file()`), ` ` otherwise
- ACL check skipped for symlinks
- Extended ACLs are entries beyond the standard `ACL_USER_OBJ`, `ACL_GROUP_OBJ`, `ACL_OTHER`
- If filesystem does not support ACLs (`EOPNOTSUPP`), treat as no ACLs

## File Handling

### Path Construction

- Command-line arguments use empty string as directory prefix
- Directory entries use the parent directory path as prefix
- Absolute paths (starting with `/`) ignore the directory prefix
- Path format: `dir/name` (or just `name` if dir is empty)

### Stat Behavior

- Always uses `lstat()` (does not follow symlinks for stat)
- Stat is lazy: only performed when first needed
- Failed stat is remembered (not retried)
- Files that fail to stat display `?` for most fields and `???????????` for modes

### Symlink Resolution

- `gettarget()`: reads one level of symlink (via `readlink()`)
- `getfinaltarget()`: follows the full chain, detecting loops via inode set
- Target paths are resolved relative to the symlink's directory
- Loop detection: if an inode is seen twice, it's a loop; print error to stderr

### Directory Listing Flow

1. Separate command-line arguments into files and directories
2. If argument is a directory (or symlink-to-directory when `-H` is active): treat as directory
3. List all files first (sorted, formatted)
4. Then list each directory:
   a. Print directory header (`dirname:`) if multiple dirs, or mixing files+dirs, or `-R`
   b. Print blank line between sections (but not before the first section)
   c. Print `total <blocks>` if `-s` or `-l`
   d. Read directory entries via `readdir()`
   e. Skip hidden files unless `-a`
   f. Apply `-D` filter (dirsonly)
   g. Sort, format, and print entries
   h. If `-R`, recurse into subdirectories

### Block Size Calculation

Blocks are stored in 512-byte (`DEV_BSIZE`) units in `st_blocks`. Conversion:
- If `blocksize > DEV_BSIZE`: `blocks = (st_blocks + factor/2) / factor` where `factor = blocksize / DEV_BSIZE` (rounds to nearest)
- If `blocksize <= DEV_BSIZE`: `blocks = st_blocks * (DEV_BSIZE / blocksize)`

Default block size: 1024 (overridable by `BLOCKSIZE` env var).

Directory totals: sum of `getblocks()` for all listed files in the directory.

## Locale

`setlocale(LC_ALL, "")` is called at startup. This affects:
- `strcoll()` for name sorting
- `mbrtowc()` / `iswprint()` / `iswcntrl()` for character classification
- `wcwidth()` for display width of wide characters
- `strftime()` for date formatting

## Error Handling

- Errors are printed to stderr with the format: `l: function: message`
- Most errors are non-fatal (program continues processing other files)
- Fatal errors (out of memory at startup, invalid options) cause `exit(1)` or `exit(2)`
- Unknown options print usage and exit with code 2
- Files that can't be stat'd are skipped (freed) when given as arguments
- Directory open failures print an error and skip that directory

## Exit Codes

- `0`: Success
- `1`: Runtime error (out of memory, etc.)
- `2`: Usage error (invalid option, missing argument)

## Incompatibilities with GNU/BSD `ls`

These options have **different meanings** from standard `ls`:

| Flag | `l` behavior | Standard `ls` behavior |
|------|-------------|----------------------|
| `-b` | Show size in bytes | C-style escaping |
| `-D` | Directories only | GNU Emacs Dired mode |
| `-g` | Show group field | Long format without owner |
| `-m` | Show modes | Stream/comma-separated format |
| `-o` | Show owner field | Long format without group |
| `-p` | Current user permissions | Append `/` to directories |
| `-H` | Follow dir links in args only | Follow all symlinks (BSD) |
| `-f` | No sorting only | No sorting + enable `-a` (GNU) |

## Environment Variables

| Variable | Effect |
|----------|--------|
| `BLOCKSIZE` | Default block size for `-s` display (parsed as integer) |
| `TERM` | Used to determine terminal color capabilities |
| `LC_ALL`, `LANG`, etc. | Locale settings affecting sort order, character handling |

## Implementation Notes

### Key Design Considerations

1. **Lazy stat**: Only stat files when metadata is first needed; cache the result.
2. **Display width vs byte width**: Track both for column alignment (wide characters, escape sequences).
3. **Color sequences**: Must contribute zero display width in column calculations.
4. **Locale-aware sorting**: `strcoll()` behavior is important for correctness.
5. **Error continuity**: Most errors should not halt the program; print to stderr and continue.
