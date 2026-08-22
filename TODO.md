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
