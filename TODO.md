# TODO

## Decisions needing review

Guesses made under autopilot, recorded so nothing decided without the
repository owner silently becomes permanent. Each says what was decided, what
the alternative was, and why it is reversible.

- [ ] **The fork-pull-request gap is documented upstream, not fixed here.**
      The shared codex-review setup is taken as-is, with its fork limitation
      recorded in `mikelward/codex-review`'s `docs/CONSUMER.md` rather than
      fixed. The alternative was holding this conversion until the shared
      action publishes its check result against `pull_request.head.sha`, so a
      fork pull request could satisfy a required `codex-review-check`.
      External fork pull requests are not a case these repositories take
      today, and the head-associated check comes from the `push` trigger,
      which same-repo pull requests always get. The three workflow files here
      are byte-identical template copies, so a local edit would fail the pin;
      the fix belongs upstream once.
      *Reversible:* entirely. When the remedy lands upstream this repository
      re-copies `templates/` and gets it for free, and the remedy is written
      out there in full — including that it is only half the gate, since a
      fork head also fails the `codex` status for a separate, deliberate
      reason.

- [ ] **`ci.yml` gains a top-level `permissions: contents: read`.** It
      declared none, so it inherited whatever the repository default grants —
      which may include `statuses: write`, and the whole point of the sole-
      writer rule is that only the codex-review sweep can write the `codex`
      status. The alternative was leaving it and relaxing the check, which
      would defeat the check. Nothing in this workflow writes anything: it
      builds and runs tests.
      *Reversible:* delete four lines. If some future job here genuinely
      needs a write scope, grant it on that job rather than at the top level.

## Add the ruleset settings the Codex gate expects

Three settings this repository's ruleset does not have yet, all explained in
the shared `docs/CONSUMER.md`: require `codex` (not `sweep`), require
`codex-review-check / codex-review-check`, and require branches to be up to
date before merging. Deliberately a follow-up — requiring a check in the same
change that installs it would block the change that installs it.

Worth knowing when the next repository is converted, since it looks like a
broken gate and is not: until `codex-review.yml` is on the default branch, the
two triggers that sweep unprompted — `schedule` and `pull_request_target` —
resolve their definition *there*, so neither fires for the pull request
installing it. What does fire is `pull_request_review_comment`, which resolves
against the merge ref, so a reply on a review thread runs the sweep and
publishes the verdict for the current head.

## Review and merge gates

- [ ] **Add `zizmor` to the ruleset's required set** once it has reported
      on a pull request: the new zizmor workflow runs unfiltered on every
      PR precisely so it can be required (a paths-filtered workflow
      creates no check run at all on a non-matching PR, which a ruleset
      waits on forever) — the posture piloted in mikelward/lanes and
      mikelward/ci-commit-artifact and rolled out fleet-wide.

- [ ] Verify the settings half of the fleet's bar — every repository works
      the same: comprehensive automated review, required merge gates, and
      auto-merge. The workflow files (CI and the codex-review set) are all
      present here; what git cannot show, and the 2026-08-18 audit could
      not verify, is the settings half: a ruleset on the default branch
      requiring the CI gate, the `codex` status, the
      `codex-review-check / codex-review-check` workflow pin, conversation
      resolution, up-to-date branches, and the auto-merge setting enabled —
      and that Codex automatic reviews are enabled for this repository:
      `codex-review.yml` only republishes an existing verdict, so with
      automatic reviews off every pull request would wait on a manual
      `@codex review` before the gate could clear.

## Decide: one rule for error messages and exit status

Undecided: "option A" from the review discussion on #52 through #54.
`file.c` prints an error whenever a lookup fails, whatever the lookup was
for, while `l.c` sets exit status 1 only at the call sites #52 and #54
added. The two drift apart: Codex found three missing sites in a row. The
proposal is that lookups stop printing, and whatever decides a failure
matters prints the message and sets the status in one call, so neither can
happen without the other. The per-site `statfailed()` checks in `l.c` would
then go. The cases to settle, each needing a test:

**Printed, but exits 0.** These would become exit 1:
- `-l` or `-V` when a link's target cannot be read (`readlink()` fails).
- ACL read errors under `-l` or `-M` ("Error getting ACLs").
- `--btime` when `statx()` fails. Separately, a file system with no birth
  times silently shows the epoch (1970) as the time.
- Running out of memory partway through a listing. SPEC.md lists it under
  exit 1, but only failures at startup set it.

**Printed for lookups that only decorate.** GNU ls prints nothing here, so
these would become silent:
- `l dangling` or `l loop` with no options. `-H` is on by default, so `l`
  looks up a link argument's target to see whether it is a directory. An
  explicit `-H` may differ: GNU reports a command-line link it was asked to
  follow and could not.
- `-F` or `-O` classifying, or `-G` coloring, a link's target name in `-l`
  and `-V` output. A dangling target already shows `?` or red.
- `-V` following a chain to a dangling end.

**Already exit 1, keep (#52, #54).** An operand that cannot be stat'd; a
directory that cannot be opened or read to the end; an entry whose own
lstat fails when the listing needs it (`?` fields, or dropped by `-D`); and
under `-L`, a final target that cannot be found or stat'd.

**Permissions.** An lstat permission failure comes from the parent
directory lacking search permission, not from the file's own mode.
- Listing only the names in a readable but unsearchable directory needs no
  lstat. Keep it silent with exit 0, as GNU ls does.
- Anything that needs each entry's metadata prints one error per entry, as
  GNU ls does. Decide whether to print one message per directory instead
  for large directories. It exits 1 either way.
- `-p` silently shows `?` for `access()` failures other than `EACCES`,
  including `EROFS` for `w` on a read-only file system. Decide whether any
  of those is an error, and whether `EROFS` should show `-`.

**Also open.**
- Exit code values. GNU ls exits 2 for an operand it cannot access and 1
  for minor problems; `l` exits 1 for every runtime failure and keeps 2
  for usage errors.
- Message format. Messages carry internal function names (`l: getstat:
  Cannot lstat ...`), where GNU ls says `ls: cannot access 'x': reason`.
