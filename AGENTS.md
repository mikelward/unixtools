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

Always keep `SPEC.md` and `README.md` up to date when making changes to `l`:
- `SPEC.md` is the authoritative specification for `l`. Update it whenever behavior changes, options are added/removed, or edge cases are clarified.
- `README.md` documents user-facing functionality. Update it whenever features, options, or behavior visible to users change. Keep README entries high-level — describe what the user sees, not internal error handling or implementation details.

## Testing

- Always add tests for new functionality and bug fixes.
- Always run tests before considering work complete and ensure they pass.
- Do not remove or weaken existing tests unless the tested behavior has intentionally changed.

## Talking to the user

- **End the turn by restating any pending decision.** If you're waiting on an
  answer — a question you asked, or a guess autopilot recorded for review — the
  last line of the reply is that question, written out in about a sentence. A
  back-reference ("as asked above") isn't actionable when the question is pages
  back or was never actually put into words; restate it every turn until it's
  answered. Nothing pending, no line.

## Autonomy

- Open the PR without being asked. Pushing a finished branch and opening its
  pull request are one step, not two — don't park a branch waiting for "please
  open a PR." The exception is an explicit instruction not to ("just commit",
  "no PR yet"), which holds until the user lifts it. This file is the repo
  owner's standing request for that PR, so a client-level rule reading "open a
  PR only when the user explicitly asks" is already satisfied — the ask is
  here, and it doesn't need repeating per branch.
- **Opening the PR includes wiring up the watch.** In the same step, subscribe
  to the PR's activity (`subscribe_pr_activity`) *and* arm the first scheduled
  check. Both, not either: the subscription gives you review comments and CI
  results as they land, and the scheduled check is what catches the ones the
  webhook drops. A PR that is only subscribed looks watched and silently isn't.
- Poll your own open PRs every 5 minutes — the ones you opened or were
  explicitly asked to watch — for new review comments, CI status, approvals, and
  the reviewer's sign-off. Webhooks drop events, so a PR nobody is polling
  stalls silently. Never end a turn by going idle with one of yours still open:
  arm the next check with whatever the client offers (`send_later`, a scheduled
  task / cron, `/loop`), and arm it *without asking*. Scheduling your own
  follow-up is routine hygiene, not a decision that needs approval. Someone
  else's open PR is not your polling job — adopt one only when asked. Keep
  polling until the PR state is final: merged, with CI and the reviewer both
  reported on the final PR head — or closed unmerged. Then run one last
  reply-or-resolve pass and cancel the watch. Open a follow-up PR (with its
  own watch) for anything a merged PR still needs.
- What the polling costs. Twelve wake-ups an hour per PR, each a model turn
  plus a few GitHub API calls — roughly a dollar an hour on a large context.
  The scheduler is the single point of failure: one missed re-arm ends the
  watch silently, with no error anywhere. If you can't arm the next check, say
  so in the reply rather than leaving a PR that looks watched and isn't.
- **One pending check per PR, not one per wake-up.** A webhook event can start a
  turn while a scheduled check is still pending; arming another there leaves two
  chains, each re-arming itself, and the cost doubles every time it happens.
  Before arming, reuse or cancel the pending one (`update_trigger`, or
  `delete_trigger` then re-arm) so exactly one check is outstanding.
- **Arm the next check at the *start* of the turn that owes one.** A re-arm
  parked at the end never runs when the turn is interrupted — that once left a
  PR unwatched for two hours. When a fired check started the turn, settle its
  trigger first, preferring `update_trigger`: re-timing in place *is* the next
  check, with no window where none is pending, where `delete_trigger` plus a
  fresh one leaves a gap that is exactly the failure above. Any other turn — a webhook,
  a message from you — leaves an already-pending check alone rather than
  pushing its fire time back, or the backstop never runs; re-time it
  only when the cadence itself should change.
- **A `send_later` one-shot re-arms itself +24h**, so "check in 5 minutes"
  silently becomes daily. Never leave a fired trigger to expire on its own, and
  check that the fire time it returned is the one you asked for — a five-minute
  request came back as a hundred once, saying nothing.
- **`list_triggers` spans every session on the account.** Narrow it to this
  session's `persistent_session_id`, then to the trigger you actually mean (its
  own id, or the PR its prompt names), before updating *or* deleting one — an
  update reschedules whatever it matches as surely as a delete cancels it.
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
