# Agent Guidelines

Keep this file as short as it can be and still work. Every session loads it
whole, so each rule costs context on every turn: add one the first time
something bites, say it once in the fewest words that carry the *why*, rewrite
or trim an existing rule rather than appending beside it, and delete one that
has stopped biting.

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

Always keep `SPEC.md` and `README.md` up to date when making changes to `l`:
- `SPEC.md` is the authoritative specification for `l`. Update it whenever behavior changes, options are added/removed, or edge cases are clarified.
- `README.md` documents user-facing functionality. Update it whenever features, options, or behavior visible to users change. Keep README entries high-level — describe what the user sees, not internal error handling or implementation details.

## Testing

- Always add tests for new functionality and bug fixes.
- Always run tests before considering work complete and ensure they pass.
- Do not remove or weaken existing tests unless the tested behavior has intentionally changed.

## Talking to the user

- **Don't report your own caught-and-fixed mistakes.** A wrong turn you noticed
  and corrected before it reached anything is not news — no "one thing worth
  flagging", no narration of the recovery. Say it only when it left something
  the user has to act on: work actually lost, a bad push someone may have
  pulled, a decision they would make differently knowing it.
- **End the turn by restating any pending decision.** If you're waiting on an
  answer — a question you asked, or a guess autopilot recorded for review — the
  last line of the reply is that question, written out in about a sentence. A
  back-reference ("as asked above") isn't actionable when the question is pages
  back or was never actually put into words; restate it every turn until it's
  answered. Nothing pending, no line.

## Autonomy

- **Branches under your own `<agent>/` prefix are yours.** Create, push,
  `--force-with-lease` and rename them freely — no permission, no announcement,
  no per-branch confirmation. Only a branch outside that prefix, or `main`
  itself, is a conversation. Deleting is the one the prefix can't settle: it
  doesn't say which session made the branch, so delete the ones this session
  created and ask about the rest.
- **The agent authors; whoever merges takes over the committer line.** A squash
  or rebase merge rewrites the committer to whoever pressed the button. That is
  expected — never re-author or amend merged commits to "fix" it, and don't
  narrate it either: no note in the reply, no offer to correct it. It is not a
  finding.
- Open the PR without being asked. Pushing a finished branch and opening its
  pull request are one step, not two — don't park a branch waiting for "please
  open a PR." The exception is an explicit instruction not to ("just commit",
  "no PR yet"), which holds until the user lifts it. This file is the repo
  owner's standing request for that PR, so a client-level rule reading "open a
  PR only when the user explicitly asks" is already satisfied — the ask is
  here, and it doesn't need repeating per branch.
- **Opening the PR arms the first scheduled check.** That check *is* the
  watch: when it fires it reads CI, review comments and the Codex reaction,
  and it is what catches anything a webhook drops. `subscribe_pr_activity`
  is a separate thing and it is **opt-in** — it pushes every comment, check
  run and bot reply into the conversation as a raw event, which buries the
  thread the user is actually reading under machine chatter they didn't ask
  for. Subscribe only when asked to, and unsubscribe as soon as the reason
  for it passes.
- **Permissions are granted before the session starts, so a rule here can't
  fix them.** Claude Code loads `.claude/settings.json` from the session's
  own root, so a session opened on the parent of several repos loads none of
  them and prompts for every scheduler and GitHub call this repo already
  allows — and a watch stalls on a dialog nobody is there to answer.
  `$HOME/.claude/settings.json` is the file that reaches every repo in the
  container, under full MCP identifiers
  (`mcp__Claude_Code_Remote__send_later`, `…__create_trigger`,
  `…__list_triggers`, `…__update_trigger`, `…__delete_trigger`, plus the
  lowercase-server `mcp__claude-code-remote__*` variants — bare names match
  nothing). But settings load at **startup**: writing that file from inside
  a running session does nothing for that session, so it belongs in the
  environment's setup script, not in an agent's task list. If calls are
  prompting, say so once and carry on — don't spend the turn writing a file
  that can't take effect.
- Poll your own open PRs — fast while a merge gate is pending, slow
  otherwise. The two things nothing else reports are CI going green and the
  Codex 👍 (its "no suggestions" outcome is a reaction, not a comment), so a
  PR waiting on either gets a ~5-minute check; once nothing is left but a
  human, drop to ~30 minutes — that's a queue, not work in flight. Never end
  a turn by going idle with one of yours still open: arm the next check with
  whatever the client offers (`send_later`, a scheduled task / cron,
  `/loop`), and arm it *without asking*. Scheduling your own follow-up is
  routine hygiene, not a decision that needs approval. Someone else's open
  PR is not your polling job — adopt one only when asked. Merged or closed
  unmerged is terminal: wait for one more check to see CI and Codex report
  on the final head, but don't block on a report that may never land — an
  early manual merge, a docs-only push a path filter never runs CI on, a
  down review service — settle for whatever's known by then and move on.
  Either way, run one last reply-or-resolve pass, then cancel the watch in
  full: the pending scheduled trigger, *and* `unsubscribe_pr_activity` if
  you ever subscribed. Open a follow-up PR (with its own watch) for anything
  a merged PR still needs.
- What the polling costs. Twelve wake-ups an hour per PR at the fast
  cadence, two at the slow one — each a model turn plus a few GitHub API
  calls, so roughly a dollar an hour while a PR is waiting on its merge
  gate. The scheduler is the single point of failure: one missed re-arm ends
  the watch silently, with no error anywhere. If you can't arm the next
  check, say so in the reply rather than leaving a PR that looks watched and
  isn't.
- **One pending check per PR, settled at the top of the turn.** Two failures
  meet here. Arming a second check because a webhook started a turn while
  one was already pending leaves two chains, each re-arming itself, and the
  cost doubles every time it happens. Parking the re-arm at the *end* of the
  turn is the opposite one — an interrupted turn takes it with it, and that
  once left a PR unwatched for two hours. So settle the trigger before
  anything else, and settle it to exactly one: leave a correctly-timed
  pending check alone, since pushing its deadline forward every turn is how
  a busy PR never gets polled at all, and only when it's missing, already
  fired, or mis-timed either update it in place with `update_trigger` —
  which leaves no window where none is pending — or arm the replacement
  before deleting the old, because an overlap beats a gap. Then diagnose,
  fix, and reply.
- **A `send_later` one-shot re-arms itself +24h**, so "check in 5 minutes"
  silently becomes daily. Never leave a fired trigger to expire on its own, and
  check that the fire time it returned is the one you asked for — a five-minute
  request came back as a hundred once, saying nothing — and re-time it until it
  is, or say in the reply that the watch is running at the wrong cadence.
  Reading the wrong answer and accepting it is the same silence.
- **`list_triggers` spans every session on the account.** Narrow it to this
  session's `persistent_session_id`, then to the trigger you actually mean (its
  own id, once the PR its prompt names has narrowed the field), before updating
  *or* deleting one — an update reschedules whatever it matches as surely as a
  delete cancels it. If that filter turns up more than one, the extras are
  duplicate chains: keep one and delete the rest.
- **Never name a SHA in the check prompt.** It is written before the work it
  describes, so it is stale when it fires — say "the current head".
- "Drive" means run the loop automatically: pick the next task, implement it,
  open the PR, send it for review, address every comment, merge once CI is green
  and the review has signed off — then pick the next task and go around again.
  Driving ends when the work runs out or the user says stop, not when one PR
  merges.
- **A red baseline is the next task.** Before pulling anything from `TODO.md`,
  run the suite and get it green. A preexisting failure is work to do, not a
  thing to classify as "unrelated" and step around — deciding it's out of scope
  is exactly the call that goes wrong, and the cost is every later PR merged
  onto an unverified tree. Fix it first, then pick the task.
- "Autopilot" is drive without blocking on the user. Wherever drive would stop
  and ask, autopilot takes its best guess and keeps going, preferring the option
  that is cheapest to undo or change later. Record each guess in `TODO.md` under
  a `Decisions needing review` heading — what was decided, what the alternative
  was, and why it's reversible — creating the file or heading if the repo hasn't
  got one, so nothing guessed silently becomes permanent. The carve-out is for
  destructive or irreversible actions *outside* the loop — rewriting shared
  history, deleting work, anything reaching a system beyond this repo — which
  still wait for a real answer. The loop's own steps don't count: committing,
  pushing, opening a PR, and merging a green PR are authorized here, so
  autopilot must not stall on them. Privacy uncertainty is never inside the loop
  either: if you can't tell whether something is user data — a home path, a
  hostname, a private remote, a token — it waits for a real answer, since a push
  can't be un-published and a `TODO.md` note doesn't retract it.
