# Feature tasks — 002 queue

Linked issue and plan: https://github.com/misja/swell/issues/4; plan.md in this directory.

- [x] Combined scope/design review by a separate agent (review_002, approve with changes); findings applied to spec and plan.
- [ ] `Status.pid`/`song`, `parsePlaylist`, with tests.
- [ ] FakePlayer: answer `/Playlist` by query (handler) and hold or delay replies.
- [ ] `QueueModel`: fixed length, windows, current window first, id/length checks, retry, active/stale. Client commands: safe one-at-a-time remove with identity check, play from, clear reset. Fixture tests per spec AC2.
- [ ] Queue page attached to Now Playing; Dutch translation including the plural.
- [ ] Build, validator, evidence/002.
- [ ] Device check with Misja, including the play-from form.
- [ ] Separate result review, then a PR.
