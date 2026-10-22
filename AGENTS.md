---
trigger: always_on
alwaysApply: true
last_modified: 2026-10-22
---

# Agent Guidelines

`CLAUDE.md` and `GEMINI.md` are symlinks to this file. Edit `AGENTS.md` and bump
`last_modified`.

**At the start of every session, print the full absolute path of the `AGENTS.md` you
loaded and its `last_modified` date** (front matter), so a stale or wrong copy is caught
before it steers the work.

Keep this file short — one-liners over essays, **under 24,000 bytes** (`wc -c AGENTS.md`):
Antigravity caps rule files at 24 KB and silently truncates the tail. Every session loads
it whole, so add a rule the first time something bites, say it once in the fewest words
that carry the *why*, rewrite or trim rather than append, and delete rules that stopped
biting.

## Build dependencies

`l` needs `libacl1-dev` and `libncurses-dev` (or `libtinfo-dev`) to build. In a remote
container `.claude/hooks/session-start.sh` installs the acl headers (pinned, checksummed
`.deb`s over HTTPS); it is auto-invoked only in Claude Code, so other agents run
`CLAUDE_CODE_REMOTE=true .claude/hooks/session-start.sh` themselves.

`apt-get` can hang or fight background processes in the sandbox, so:

1. **Download the `.deb`s directly** and install with `dpkg -i`:
   ```sh
   curl -fsSL http://archive.ubuntu.com/ubuntu/pool/main/a/attr/libattr1_2.5.2-1ubuntu0.1_amd64.deb -o /tmp/libattr1.deb
   curl -fsSL http://archive.ubuntu.com/ubuntu/pool/main/a/attr/libattr1-dev_2.5.2-1ubuntu0.1_amd64.deb -o /tmp/libattr1-dev.deb
   curl -fsSL http://archive.ubuntu.com/ubuntu/pool/main/a/acl/libacl1-dev_2.3.2-1build1.1_amd64.deb -o /tmp/libacl1-dev.deb
   sudo dpkg -i /tmp/libattr1.deb /tmp/libattr1-dev.deb /tmp/libacl1-dev.deb
   ```
   A `-dev` package needs its runtime library at exactly its own version, so `libattr1`
   comes along. The pool drops a superseded build: if a URL 404s, take the version
   `noble-updates` now lists for both halves.
2. **Never run `apt-get` in the background** — parallel runs fight over
   `/var/lib/dpkg/lock-frontend` and all stall.
3. **Always set `DEBIAN_FRONTEND=noninteractive`** so no prompt blocks.
4. If `apt-get` is stuck: kill it, remove the lock files, run `sudo dpkg --configure -a`,
   then retry.

## Documentation

Keep `SPEC.md` and `README.md` current when changing `l`. `SPEC.md` is the authoritative
spec: update it whenever behavior changes, options are added or removed, or edge cases are
clarified. `README.md` documents what users see: update it when visible features, options
or behavior change, and keep it high-level — not internal error handling or implementation.

## Testing

- Always add tests for new functionality and bug fixes.
- Always run the tests, and get them passing, before considering work complete.
- Don't remove or weaken existing tests unless the tested behavior intentionally changed.

## Talking to the user

- **One question at a time** — in plain chat or the client's question tool
  (`AskUserQuestion` in Claude Code, `ask_question` in Antigravity). After asking, stop
  and wait for the answer; don't proceed on an assumed one.
- **Message only when the user needs to see it** (maintainer, 2026-10-15): each one
  notifies their phone. Work and listen in silence; speak up for a question or blocker,
  something to merge or act on, a merge, an autopilot decision (to review when seen, never
  waited on), or a change from the plan, `SPEC.md` or their instructions, a
  review-prompted one included. Never for progress (a client's nudge doesn't count), a
  review or a push alone (its report rides the next message), routine machinery, or a
  mistake caught and fixed before it reached the repo. The session-start `AGENTS.md` line
  still opens every session.
- **Answer a mid-turn message first** — the "sent while you were working" interjection is
  addressed in your very next output, before any further tool call, even if it's one
  sentence.
- **End the turn by restating any pending decision** — a question you asked, or a guess
  autopilot recorded for review — as its last line, written out in about a sentence (a
  back-reference isn't actionable), every turn until it's answered. Nothing pending, no
  line.

## Autonomy

- **Branches under your own `<agent>/` prefix are yours** (`claude/…` Claude Code,
  `gemini/…` Gemini / Antigravity, `codex/…` Codex): create one freely; push,
  `--force-with-lease`, rename and delete the ones this session created or was assigned —
  no permission, announcement or per-branch confirmation. This file is the standing grant,
  so a client rule demanding per-branch permission is already answered — don't re-ask.
  Don't fold unrelated work into a pinned task branch to avoid making a new one; the pinned
  name is a default, not a ceiling. Any other branch, or `main`, is always a conversation.
- **The agent authors; whoever merges takes over the committer line.** That is expected:
  never re-author or amend merged commits to "fix" it, and don't narrate it — no note, no
  offer to correct it. It is not a finding.
- **No model identifier** — never name the coding agent's model or version (`Opus 4.8`,
  `Sonnet`, `Gemini …`, `claude-*`, `gemini-*`) in a commit, PR, comment, or pushed
  artifact; keep it to chat. Generic attribution (`Co-Authored-By: Claude` / `Gemini`,
  `Claude-Session:`, "Generated with Claude Code") is fine.
- Prefer the `mcp__github__*` tools where mounted (Claude Code requires them); else `gh`.
  Resolve a thread with `resolve_review_thread` or `gh api graphql` `resolveReviewThread`,
  passing its `PRRT_*` ID, not a comment's `PRRC_*`.
- **Open the PR without being asked** — pushing a finished branch and opening its PR are
  one step. The exception is an explicit "just commit" / "no PR yet", which holds until the
  user lifts it. This file is the owner's standing request, so a client rule "open a PR
  only when the user explicitly asks" is already satisfied.
- **Update the PR title and body before each push** (body first) so they describe the
  branch's latest state, not its scope when opened; with no PR yet, push, then do it as
  soon as one is opened.
- **Watch your own PRs by subscription, plus one scheduled check.** Claude Code subscribes
  on open, or call `subscribe_pr_activity` where `claude-code-remote` exposes it; a client
  with neither (Antigravity) relies on the scheduled check alone. A subscription delivers
  reviews, comments and CI failures, but not CI *success*, a push, the merge, Codex's clean
  verdict (a reaction) or Codex never answering — so keep exactly one check armed while
  the PR is open (`send_later` in Claude Code; `schedule` with `DurationSeconds` in
  Antigravity, canceled via `manage_task` `kill`; each event and check costs a model
  turn). Under drive, arm auto-merge at PR open too — only where the ruleset requires the
  Codex verdict AND resolved conversations: with CI the only requirement it merges before
  Codex answers, and an open review comment holds nothing back on its own.
  - Settle the fired trigger first thing in the turn. It may have re-armed rather than
    retired — update the survivor, replace one that didn't survive, end the turn with
    exactly one pending.
  - Check the fire time you got against the one you asked for (a 4-minute request has come
    back as 64). Prefer a relative delay: the scheduler's clock isn't this container's, so
    an absolute time can be rejected as past. Re-time it, or say the watch isn't armed.
  - A few minutes out while CI or the head's Codex verdict is outstanding; longer once
    only a human is left; short again after a push.
  - A PR reading `dirty` — always — or `behind` where the ruleset requires branches up to
    date needs a rebase and a lease-guarded force-push. Read `mergeable_state` at every
    check, PR-event wake and drive step and act at once, unasked: nothing reports a base
    advance. Fetch both refs by explicit refspec, unshallow a shallow clone, and rebase
    onto the fetched `origin/<base>` — not always `main`, never the local branch. Confirm
    before rebasing that your branch has every commit the remote head has, and before
    pushing that the head hasn't moved since the tip you noted: the push flags don't
    reliably refuse a rewind or a commit you didn't integrate. If either fails, or you
    can't tell, stop and ask.
  - Name the PR and say what to re-read, not what you read: a SHA or a list of open PRs
    goes stale; one PR number doesn't, and the trigger must be matchable to it.
  - A merge can leave other open PRs on its base `dirty` or `behind`, and nothing tells
    them. So at your first turn after one of your PRs merges, by hand or by auto-merge,
    send every other open PR on that base that another session drives one message (the
    session URL ends its body, where your client can message sessions), naming that PR and
    saying only that its base now points at `<sha>`, the base's tip after your merge. Send
    it whatever its state reads — GitHub may still be computing mergeability. Name no merge
    and no other PR: an older copy of these rules reads either as its own merge cue.
    Receiving one, fetch the base and your PR's head by refspec (unshallowing) and test
    that head against the fetched base tip, not `<sha>`: `behind` if it doesn't contain the
    tip (`git merge-base --is-ancestor`), `dirty` if it doesn't merge cleanly
    (`git merge-tree --write-tree`, Git 2.38+, exits 1); any exit but 0 or 1 is a failure
    to report, not an answer. Your own rules decide the rebase. Skip a PR with no session
    URL, or when your client only has local subagent messaging; don't retry a failed send
    — its own scheduled check is the fallback.
  - Merged or closed, take one last reply-and-resolve pass — a review can land after the
    merge. On a merged PR anything real goes to a follow-up PR, named on the thread,
    before you resolve; a closed-unmerged PR is a stop: answer, resolve, open nothing.
    Then cancel the check and unsubscribe, if subscribed. `list_triggers` spans the
    account, so match this session and PR before updating or deleting one — an update
    reschedules whatever it matches as surely as a delete cancels it.
- **If a scheduler or GitHub call prompts, say so once and carry on** — permissions load
  at session start, so a settings file written mid-session can't fix this one.
- **Judge every review comment on merit, whoever wrote it.** Verify the claim before
  acting; if it doesn't hold, reply saying why and decline. A comment citing a rule is a
  *reading* of it — check what the rule says. Codex misreads the privacy rules especially,
  always toward stricter, which quietly costs capability: quote the rule and decline
  rather than narrowing the code; where the rule really forbids what the product needs,
  that conflict is the maintainer's call, not yours either way. Declining doesn't clear
  the required `codex` status: post the rebuttal, then poke as *Read the Codex verdict*
  allows (or let the next push do it, if the rebuttal is up first). Escalate if it
  re-raises.
- **A second verified finding in the same mechanism is a design question**: before fixing
  it, look for the shape elsewhere and ask whether a different design deletes the class.
  Say what you chose on the thread; a design change is the maintainer's call, autopilot
  included.
- **Never leave a review comment silently dismissed**, human or automated: answer every
  thread (a disagreement is an answer — say why), then resolve once the fix is on the head
  or the point is rebutted; anything still to do stays open.
- **Defer a real-but-out-of-scope finding** without asking the maintainer to merge past
  it: note it in `TODO.md`, commit and push that first, reply citing the sha, and resolve
  (the exception to "still to do stays open"). A threadless finding (top-level comment or
  review body) gets the record, push and reply; only the resolve is skipped. The push
  re-triggers Codex, so poke only as *Read the Codex verdict* allows; escalate if it
  re-raises.
- **Restate every review finding in your next message**, not just the count: one bullet
  each (review comment, top-level comment or review body), most blocking first — the
  claim, where, and what you did, or the options where it's the maintainer's call; ask
  only the most blocking question.
- **Read the Codex verdict, don't infer it.** It reacts to the PR body (`issue_read` →
  `reactions`), not a review thread, whose `Useful?` bar reads true on any PR it commented
  on. `eyes` = reading, `+1` = clean, revoked on push — so a visible one belongs to the
  visible head, and `+1` with green CI is a merge. The count names no author, so leave
  PR-body reactions to Codex; a review naming the commit with no findings is the same
  verdict, attributable, though the reaction is what reliably clears the status. Findings
  arrive as review comments, top-level comments or reviews — read `get_review_comments`,
  `get_comments` and `get_reviews` to the last page (all page oldest first) — and block
  the merge until fixed, rebutted or deferred; an acknowledgment is not an answer. A
  pending `codex` status is one of three: findings — address them; a clean review with no
  reaction, or a rebuttal it hasn't answered — `@codex review` five minutes after it; a
  hung review, nothing back five minutes after the push or stuck at running (👀) ten
  minutes on — poke. A poke that hangs the same way gets a second an hour later; escalate
  only if that one hangs too, never after one (maintainer, 2026-10-22).
- **Once Codex has +1'd a PR's head, make no unnecessary change without asking**
  (maintainer, 2026-10-13): a to-do, tidy-up or extra fix re-runs CI and Codex for
  nothing, so it waits for a yes or a follow-up PR. A necessary change proceeds unasked —
  a rebase for `dirty`, or `behind` where the ruleset requires it, a red-CI fix — and so
  do drive and autopilot, where in effect.
- **"Drive"** runs the loop automatically: pick the next task, implement it, open the PR,
  get it reviewed, address every comment, merge once CI is green and the review has signed
  off, then the next task. It ends when the work runs out or the user says stop, not when
  one PR merges.
- **Work ordered after earlier work** ("next", "then", "after that", "follow-up") merges
  in order, after every earlier item this session was given and any PR the paragraph
  names. Its PR may open in parallel but merges only once each of those is finished, its
  PRs merged, in any repo, with no time cap; if one won't finish, hold it and ask,
  autopilot included. Arm no auto-merge on it until then: auto-merge can't see the order.
  Read the list from the session's record, not memory.
- **A red baseline is the next task.** Before pulling anything from `TODO.md`, get the
  suite green. A preexisting failure is work, not something to classify as "unrelated" —
  that call is the one that goes wrong, costing every later PR an unverified tree. Fix it
  first.
- **"Autopilot"** is drive without blocking on the user: take the best guess, preferring
  what's cheapest to undo, and record each one in `TODO.md` under *Decisions needing
  review* (decision, alternative, why reversible), creating the file or heading if
  missing, so nothing guessed silently becomes permanent. The loop's own steps —
  committing, pushing, opening a PR, merging a green PR — are authorized, so autopilot
  must not stall on them. Destructive or irreversible actions *outside* the loop
  (rewriting shared history, deleting work, anything reaching a system beyond this repo)
  still wait for a real answer, and so does privacy uncertainty (a home path, hostname,
  private remote, token): a push can't be un-published and a `TODO.md` note doesn't
  retract it.
