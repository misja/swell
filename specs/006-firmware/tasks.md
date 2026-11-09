# Tasks — 006 firmware updates

- [x] Explore the API, azzurro's notes and the NAD (read-only); Misja: firmware only, upgrade=check acceptable, in-app web view (2026-11-09).
- [x] Draft spec and plan.
- [x] Scope/design review by a separate agent (approve with changes); findings applied to spec and plan.
- [x] Misja approved (2026-11-09); installed in both SDK targets: qtmozembed-qt5-devel 1.55.10, sailfish-components-webview-qt5(-popups) 1.6.9 (zypper noted NOKEY for the Jolla package signature).
- [x] `http://<NAD>/upgrade?noheader=1`: 200, the same page without the header, "No update available." (2026-11-09).
- [x] Misja: require sailfish-components-webview-qt5-popups too (2026-11-09).
- [ ] Issue.
- [x] UI on the design canvas, agreed with Misja (2026-11-09): notice on Home only; Home pulley "Settings" for the player's settings, firmware first; update page with the player's name.
- [x] Check upstream WebView practice in the target: documented WebViewPage pattern, no main() change, qt5embedwidget link, package-name requirements.
- [x] Tests, library, app and packaging; make check passes (11 suites).
- [x] Validator rpath WARNING researched (evidence.md): documented link for WebView 1.6.9, warning accepted pending Misja; drop the link with WebView 1.8.
- [ ] Dutch translations of the new strings.
- [ ] Builds (aarch64, armv7hl), validator, evidence.md.
- [ ] Device check with Misja.
- [ ] Separate result review, then a PR.
