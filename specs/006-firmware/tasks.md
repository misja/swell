# Tasks — 006 firmware updates

- [x] Explore the API, azzurro's notes and the NAD (read-only); Misja: firmware only, upgrade=check acceptable, in-app web view (2026-11-09).
- [x] Draft spec and plan.
- [x] Scope/design review by a separate agent (approve with changes); findings applied to spec and plan.
- [ ] Misja: install qtmozembed-qt5-devel (and the webview packages) into the SDK target (installation rule).
- [ ] Misja: read-only check of `http://<NAD>/upgrade?noheader=1`.
- [ ] Misja: require sailfish-components-webview-qt5-popups too (JavaScript dialogs on the player's page)?
- [ ] Issue.
- [ ] UI on the design canvas (notice placement, web page), agreed with Misja.
- [ ] Check upstream WebView practice (permission, requirement, initialisation) in the target.
- [ ] Tests first (fake player: check answers, upgrade stages), then library, app, packaging.
- [ ] Builds (aarch64, armv7hl), validator, evidence.md.
- [ ] Device check with Misja.
- [ ] Separate result review, then a PR.
