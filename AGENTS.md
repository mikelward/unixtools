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
- **Opening the PR arms the watch, and it has two halves.** The events
  GitHub pushes into the conversation are the fast one — opening a PR
  subscribes this session automatically, and that stays on until the PR is
  merged or closed. The scheduled check is the slow one, and it is what
  catches whatever the webhooks drop. Don't unsubscribe to quiet the thread,
  and don't relay it either: an event that needs no action — your own reply
  echoing back, a deploy-preview bot, a check run that passed — ends the
  turn with **no reply at all**. Not a summary, not a note that you are
  skipping it. Saying "deploy preview, no action" is the noise, not the
  filter.
- **If a scheduler, GitHub or `git push` call prompts, say so once and carry on.**
  Permissions load at session start, so writing a settings file mid-session
  can't fix the session you're in.
- Poll your own open PRs: one check five minutes after a push, then ~30
  minutes. The subscription delivers CI and review activity within seconds,
  so a standing five-minute loop buys nothing and costs a turn every time it
  fires. What it cannot deliver is Codex never picking the push up: no
  review, no reaction, no event, and silence that looks identical to
  still-reading. That is what the five-minute check is for — nothing from
  Codex by then means comment `@codex review`, once. After that the webhooks
  carry it and the slow check is the backstop for what they drop. Never end
  a turn idle with one of yours open: arm the next check with whatever the
  client offers (`send_later`, a scheduled task / cron, `/loop`), and arm it
  *without asking*. Someone else's PR is not your polling job unless you're
  asked. Merged or closed is terminal: take one more check for CI and Codex
  on the final head, settle for what's known if a report may never land,
  then run a last reply-or-resolve pass and cancel the watch in full — the
  pending trigger and that PR's subscription (`unsubscribe_pr_activity`
  takes one PR, so it leaves any other watch alone). Open a follow-up PR,
  with its own watch, for anything a merged one still needs.
- Read the Codex verdict, don't infer it. It reacts to the PR body
  (`issue_read` → `reactions`), not to a review thread, whose `Useful?` bar
  reads true on any PR it has commented on. `eyes` means reading, `+1` means
  clean, and Codex revokes it on push — so a visible one belongs to the
  visible head, and `+1` with green CI is a merge. The count names no
  author, so leave PR-body reactions to Codex: nobody else's is revoked, and
  a review naming that commit with no findings is the same verdict, in the
  attributable form. Findings arrive as review comments, as a top-level
  comment, or as a review — read `get_review_comments`, `get_comments` and
  `get_reviews` to the last page, since all three page oldest first — and
  they block the merge until fixed or rebutted; an acknowledgement is not an
  answer. Nothing from Codex since the push, five minutes on, means it never
  picked it up — comment `@codex review`, once.
- What the polling costs. Two wake-ups an hour per PR, plus one per push —
  each a model turn and a few GitHub API calls. The scheduler is the single
  point of failure: one missed re-arm ends the watch silently, with no error
  anywhere. If you can't arm the next check, say so in the reply rather than
  leaving a PR that looks watched and isn't.
- **One pending check per PR, settled at the top of the turn.** Two chains
  each re-arming themselves double the cost every time a webhook starts a
  turn while one is already pending; parking the re-arm at the *end* of the
  turn loses it when the turn is interrupted, which once left a PR unwatched
  for two hours. So settle it first, and settle it to exactly one: leave a
  correctly-timed check alone — pushing its deadline forward every turn is
  how a busy PR never gets polled — and when it's missing, already fired, or
  mis-timed, either `update_trigger` it in place or arm the replacement
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
- **Never name a SHA — or a list of PR numbers — in the check prompt.** Both
  are written before the work they describe, so both are stale when it
  fires, and a queued firing carries the prompt as it was when it was
  queued: editing it mid-turn does not reach a check already on its way.
  Name what to re-read.
- **The scheduler's clock is not this container's.** An absolute
  `run_once_at` computed from `date` here can be rejected as already past —
  prefer a relative delay where the client offers one, or read the
  scheduler's clock and leave margin.
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
