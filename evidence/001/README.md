# Evidence — 001 core controller

| Date | Check | Result | Log |
|---|---|---|---|
| 2026-11-06 | `sfdk build` (SDK 3.13.5, SailfishOS-5.1.0.11-aarch64, no-pull-build-requires), scaffold only | exit 0; RPM sha256 7da2de0ace56fea7ca86f616289eae5964792fe957d263fb7d4de1e936bd4e3d | sdk-build-aarch64.log |
| 2026-11-06 | `sfdk check -s harbour` (sdk-harbour-rpmvalidator 1.110) | succeeded; 1 warning: `/usr/bin/harbour-swell` not stripped (open) | harbour-validator.log |

First validator run failed: `%license` installed to `/usr/share/licenses`, which
Harbour does not allow; LICENSE is now installed in `/usr/share/harbour-swell`.
Build tree: `/home/ubuntu/swell-001` in the `sailfish-sdk` container.
| 2026-11-06 | QtTest spike `tests/spike` via `sfdk build-shell` + `make check` (aarch64, qemu-user) | 5 passed, 0 failed; loopback TCP+HTTP via QNetworkAccessManager, UDP datagram, `qWait(200)` = 207 ms. i486 fallback not needed | spike-aarch64.log |

`qt5-qttest-devel` was already present in the build snapshot
(`SailfishOS-5.1.0.11-aarch64.default`); `zypper in` reported "Nothing to do",
so no package was installed (approval granted by Misja 2026-11-06).
| 2026-11-06 | Unit tests `tests/` (spike, lsdp, xml) via `make check`, aarch64 | spike 5/5, lsdp 18/18, xml 14/14 passed | unit-aarch64.log |
| 2026-11-06 | + client tests (`tests/client`, in-process FakePlayer), aarch64 | spike 5/5, lsdp 18/18, xml 14/14, client 14/14 passed | unit-aarch64.log |
| 2026-11-06 | Mutation check: rate gate disabled in a build copy | `oneSecondRule` (gap 29 ms) and `volumeBurstCollapses` failed as expected; source restored (cmp identical) | — |
