# Tasks — 006 firmware updates

- [x] Explore the API, azzurro's notes and the NAD (read-only); the maintainer: firmware only, upgrade=check acceptable, in-app web view (2026-11-09).
- [x] Draft spec and plan.
- [x] Scope/design review by a separate agent (approve with changes); findings applied to spec and plan.
- [x] the maintainer approved (2026-11-09); installed in both SDK targets: qtmozembed-qt5-devel 1.55.10, sailfish-components-webview-qt5(-popups) 1.6.9 (zypper noted NOKEY for the Jolla package signature).
- [x] `http://<NAD>/upgrade?noheader=1`: 200, the same page without the header, "No update available." (2026-11-09).
- [x] the maintainer: require sailfish-components-webview-qt5-popups too (2026-11-09).
- [x] Issue: misja/swell#28.
- [x] UI on the design canvas, agreed with the maintainer (2026-11-09): notice on Home only; Home pulley "Settings" for the player's settings, firmware first; update page with the player's name.
- [x] Check upstream WebView practice in the target: documented WebViewPage pattern, no main() change, qt5embedwidget link, package-name requirements.
- [x] Tests, library, app and packaging; make check passes (11 suites).
- [x] Validator rpath WARNING researched (evidence.md): documented link for WebView 1.6.9, warning accepted by the maintainer (2026-11-09); drop the link with WebView 1.8.
- [x] Dutch translations of the new strings.
- [x] Builds (aarch64, armv7hl), validator, evidence.md (ea43753).
- [x] Device check with the maintainer (2026-11-09): passes, against the test player and the NAD page.
- [x] Separate result review (approve with changes); findings resolved (evidence.md).
- [x] Separate review of the changes after the device check (direct update page, "Start update", the polling fix).
- [ ] PR.
