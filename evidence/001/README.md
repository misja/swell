# Evidence — 001 core controller

| Date | Check | Result | Log |
|---|---|---|---|
| 2026-11-06 | `sfdk build` (SDK 3.13.5, SailfishOS-5.1.0.11-aarch64, no-pull-build-requires), scaffold only | exit 0; RPM sha256 7da2de0ace56fea7ca86f616289eae5964792fe957d263fb7d4de1e936bd4e3d | sdk-build-aarch64.log |
| 2026-11-06 | `sfdk check -s harbour` (sdk-harbour-rpmvalidator 1.110) | succeeded; 1 warning: `/usr/bin/harbour-swell` not stripped (open) | harbour-validator.log |

First validator run failed: `%license` installed to `/usr/share/licenses`, which
Harbour does not allow; LICENSE is now installed in `/usr/share/harbour-swell`.
Build tree: `/home/ubuntu/swell-001` in the `sailfish-sdk` container.
