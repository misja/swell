# Feature tasks — 002 queue

Linked issue and plan: https://github.com/misja/swell/issues/4; plan.md in this directory.

- [x] Combined scope/design review by a separate agent (review_002, approve with changes); findings applied to spec and plan.
- [x] `Status.pid`/`song`, `parsePlaylist`, with tests.
- [x] FakePlayer: answer `/Playlist` by query (handler) and hold or delay replies.
- [x] `QueueModel`: fixed length, windows, current window first, id/length checks, retry, active/stale. Client commands: safe one-at-a-time remove with identity check, play from, clear reset. Fixture tests per spec AC2.
- [x] Queue page attached to Now Playing; Dutch translation including the plural.
- [x] Build, validator, evidence/002.
- [x] Device check with Misja, including the play-from form.
- [x] Separate result review (and design review of the id adoption), findings resolved.
- [ ] PR for maintainer review.
