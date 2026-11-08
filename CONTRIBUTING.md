# Contributing to Swell

Contributions are welcome: bug reports, fixes, translations and features.
You may work with an AI assistant; the same rules apply, see below.

## Small changes

For a bug fix, a translation or a small improvement, open a pull request
directly. Say what it changes and how you checked it.

## Features

Features are spec-driven. Each has its own directory under `specs/`, for
example `specs/003-mpris/`:

- `spec.md`: the problem, scope, what is out of scope, and testable
  acceptance criteria;
- `plan.md`: how it will be built, and the alternatives considered;
- `tasks.md`: the steps, ticked off as they are done;
- `evidence.md`: what was checked and the results (tests, builds, device
  checks, reviews).

The usual route:

1. Open an issue and agree the scope with the maintainer.
2. Write the spec and plan. Look at how the platform and upstream already
   solve the problem before adding anything new.
3. For a change to the UI, agree how it looks first, in the issue.
4. Build it, with tests.
5. Record the results in `evidence.md`.
6. Open a pull request that links the issue.

The existing specs are worked examples.

## Code

- `bluos/` is a Qt Core and Network library for the BluOS control API. It has
  no QML or UI code.
- `src/` holds the app and what the QML pages need from the library.
- `qml/` holds the pages.
- C++ follows the Qt Coding Style. QML follows the
  [Sailfish OS coding conventions](https://docs.sailfishos.org/Develop/Apps/Coding_Conventions/).
- Code, comments and documents are in English. User-visible text goes
  through `qsTr`; translations are in `translations/`.

## Building and testing

Use the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/). From
the repository, with a build target selected:

```
sfdk build
sfdk build-shell sh -c "mkdir -p build/tests && cd build/tests && qmake ../../tests/tests.pro && make && make check"
```

All test suites must pass. New behaviour comes with tests. The tests use an
in-process fake player, so no real player is needed.

## Working with an AI assistant

`AGENTS.md` has the instructions for coding assistants. In short:

- Say in the pull request which parts were written with an AI assistant.
- Only report tests and checks that actually ran, with their results.
- A review by an AI assistant is not a human review. The maintainer reviews
  every change before it is merged.
- Never commit secrets, credentials or data from real devices; use synthetic
  values.

## Releases

The maintainer decides on merges and releases. A release is an annotated tag
`vX.Y.Z` with a GitHub release.
