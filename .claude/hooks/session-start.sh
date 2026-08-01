#!/bin/bash
# Install the build dependencies `l` needs. Without libacl the tree does not
# compile at all (file.c includes <sys/acl.h>), so a web session starts unable
# to run `make` or any test.
#
# apt-get deadlocks against background processes in this sandbox, so fetch the
# .debs directly and install with dpkg — the approach AGENTS.md documents.
# These go to `dpkg -i` as root, so fetch over HTTPS and verify a pinned
# checksum first: without that, anything that can modify the response gets
# root-level file writes and maintainer-script execution. Bump the versions and
# their checksums together.
set -euo pipefail

# Local checkouts already have their dependencies; only the ephemeral remote
# container starts bare.
if test "${CLAUDE_CODE_REMOTE:-}" != "true"; then
    exit 0
fi

# libncurses-dev is the third build dependency and ships in the image, so
# nothing here installs it. Checked before the acl short-circuit below, or the
# common case never reaches it — and if the image ever stops shipping it, the
# resulting link error points at ncurses rather than at this hook.
if ! test -f /usr/include/ncurses.h && ! test -f /usr/include/curses.h; then
    echo "session-start: ncurses headers are missing; \`make\` will fail to link" >&2
fi

# Already installed — whether the image ships them or an earlier session put
# them there. Print what is present and leave it alone rather than comparing
# against a pin: the version the build actually used is then on the record,
# and nothing here overwrites a package the environment chose.
# Overridable so the test can drive the install path without a machine that
# happens to be missing the headers.
acl_header=${ACL_HEADER:-/usr/include/sys/acl.h}

if test -f "$acl_header"; then
    echo "session-start: using the installed acl headers: $(dpkg-query -W -f='${Version}' libacl1-dev 2>/dev/null)" >&2
    exit 0
fi

mirror=https://archive.ubuntu.com/ubuntu/pool/main
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# path <TAB> sha256
debs="a/attr/libattr1-dev_2.5.2-1build1.1_amd64.deb	9a46b3d570e8992ff5ee4631abb4ee42e0e26f3febb01564e795703fe2c649f2
a/acl/libacl1-dev_2.3.2-1build1.1_amd64.deb	c9711e29621acc8abb01a337d147e38288f950c75e3f761dbdbbfc634d4a7bdf"

while IFS=$'\t' read -r path sha; do
    deb=$(basename "$path")
    # Bounded: this runs before the session is usable, so a mirror that
    # accepts the connection and then stalls would hold up startup with
    # nothing on screen to say why.
    if ! curl -fsSL --proto '=https' --tlsv1.2 \
            --connect-timeout 10 --max-time 300 \
            "$mirror/$path" -o "$tmp/$deb"; then
        echo "session-start: failed to download $deb; \`make\` will not build" >&2
        exit 1
    fi
    if ! (cd "$tmp" && echo "$sha  $deb" | sha256sum --check --strict -); then
        echo "session-start: $deb failed checksum; refusing to install it" >&2
        exit 1
    fi
done <<EOF
$debs
EOF

# DEBIAN_FRONTEND goes after `sudo`, not before: sudo builds a fresh
# environment, so an exported value set out here never reaches dpkg. Without
# it a maintainer script that calls debconf waits forever on a prompt nobody
# is there to answer, and the session hangs at startup.
if ! sudo DEBIAN_FRONTEND=noninteractive dpkg -i "$tmp"/*.deb; then
    echo "session-start: dpkg failed to install the acl headers" >&2
    exit 1
fi
