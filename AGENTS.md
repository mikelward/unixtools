# Agent Guidelines

## Build Dependencies

This project requires `libacl1-dev` and `libncurses-dev` (or `libtinfo-dev`) to build.

When installing packages in the Claude Code sandbox, `apt-get` can hang or
conflict with background processes. To avoid this:

1. **Download `.deb` files directly** with `curl`/`wget` and install with `dpkg -i`:
   ```sh
   curl -sL http://archive.ubuntu.com/ubuntu/pool/main/a/attr/libattr1-dev_2.5.2-1build1.1_amd64.deb -o /tmp/libattr1-dev.deb
   curl -sL http://archive.ubuntu.com/ubuntu/pool/main/a/acl/libacl1-dev_2.3.2-1build1.1_amd64.deb -o /tmp/libacl1-dev.deb
   sudo dpkg -i /tmp/libattr1-dev.deb /tmp/libacl1-dev.deb
   ```
2. **Never run `apt-get` in the background** — multiple `apt-get` processes
   will fight over `/var/lib/dpkg/lock-frontend` and all stall.
3. **Always use `DEBIAN_FRONTEND=noninteractive`** to prevent interactive
   prompts from blocking.
4. If `apt-get` is stuck, kill the process, remove the lock files, run
   `sudo dpkg --configure -a`, then retry.

## Documentation

Always keep `spec.md` and `README.md` up to date when making changes to `l`:
- `spec.md` is the authoritative specification for `l`. Update it whenever behavior changes, options are added/removed, or edge cases are clarified.
- `README.md` documents user-facing functionality. Update it whenever features, options, or behavior visible to users change.

## Testing

- Always add tests for new functionality and bug fixes.
- Always run tests before considering work complete and ensure they pass.
- Do not remove or weaken existing tests unless the tested behavior has intentionally changed.
