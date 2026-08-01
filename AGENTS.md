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
  else's open PR is not your polling job — adopt one only when asked. Merging
  doesn't end the watch either: reviewers and bots comment afterward, so drop to
  a slower cadence (every half hour or so) until every late comment is handled
  *and* the PR has gone about a day without a new one. Both, not either: at the
  moment of merge "every comment handled" is vacuously true, so the quiet window
  has to actually elapse.
- Three polling states, so the 5-minute cadence has an end. Five minutes is for
  a PR with something outstanding: CI running, a review requested, a comment
  unanswered, a merge conflict. Once a PR is green, reviewed, and has nothing
  left but the merge — or is merged and only waiting out late comments — drop to
  half-hourly. Stop entirely when it merges or closes and the late-comment window
  has passed. A PR that is green and waiting on a human overnight gets the slow
  cadence, not the fast one; at roughly a dollar an hour the fast cadence is for
  work in flight, not for a queue.
- What the polling costs. Twelve wake-ups an hour per PR at the 5-minute
  cadence, each one a model turn plus a handful of GitHub API calls. The API
  calls are free of concern — trivial against the 5,000 requests/hour
  authenticated limit. The model turns are the real cost: each re-reads the
  conversation, so at Opus rates ($5 per million input tokens, $25 output, with
  cached input reading at roughly a tenth of the input rate) a wake-up on a
  large context runs on the order of ten cents, i.e. ~$1/hour per PR watched.
  That is why the cadence drops as soon as nothing is pending. Owning the PR is
  itself the decision to keep the slow-cadence watch running, overnight
  included — don't ask for that. What's worth raising is holding the *fast*
  cadence open unattended: when something has been outstanding for hours with
  nothing moving, say so and drop to half-hourly rather than billing a dollar
  an hour against a stalled queue. The
  scheduler is the single point of failure: one missed re-arm ends the watch
  silently, with no error anywhere. If you can't arm the next check, say so in
  the reply rather than leaving a PR that looks watched and isn't.
- **One pending check per PR, not one per wake-up.** A webhook event can start a
  turn while a scheduled check is still pending; arming another there leaves two
  chains, each re-arming itself, and the cost doubles every time it happens.
  Before arming, reuse or cancel the pending one (`update_trigger`, or
  `delete_trigger` then re-arm) so exactly one check is outstanding.
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
