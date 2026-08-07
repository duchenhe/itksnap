# ITK-SNAP Patched macOS distribution

The distribution keeps the patched build separate from an upstream ITK-SNAP
installation:

- application filename: `ITK-SNAP-Patched.app`
- Finder display name: `ITK-SNAP Patched`
- bundle identifier: `org.itksnap.patched`
- normal install path: `/Applications/ITK-SNAP-Patched.app`

The current dependency set is arm64-only and requires macOS 26 or newer.  The
main executable supports older systems, but one or more of the current VTK
libraries were built with a macOS 26 deployment target, so declaring an older
minimum would produce an installation that can fail at launch.

## Local test package

Run:

```sh
./script/package_macos_distribution.sh
```

Without a signing identity, this creates an ad-hoc-signed file whose name ends
in `-UNNOTARIZED.dmg`.  It is for local testing only and is intentionally easy
to distinguish from a distributable release.

## Developer ID signed and notarized package

Install a `Developer ID Application` certificate and its private key in the
login keychain.  Store notarization credentials without placing the password in
this repository:

```sh
xcrun notarytool store-credentials "itksnap-patched-notary" \
  --apple-id "APPLE_ID" \
  --team-id "TEAM_ID" \
  --password "APP_SPECIFIC_PASSWORD"
```

Move the existing `build/distribution/macos-arm64` staging directory aside,
then run with the exact identity shown by `security find-identity`:

```sh
MACOS_SIGN_IDENTITY="Developer ID Application: NAME (TEAM_ID)" \
MACOS_NOTARY_PROFILE="itksnap-patched-notary" \
./script/package_macos_distribution.sh
```

The signed branch enables Hardened Runtime, signs nested code and the DMG with
the same Developer ID identity, submits the DMG to Apple, waits for the result,
staples the ticket, validates it, and runs a Gatekeeper assessment.  Its output
is `ITK-SNAP-Patched-4.4.0-macOS-arm64.dmg` with no `UNNOTARIZED` suffix.

Do not copy the application-group entitlement from the upstream release.  It
belongs to the upstream author's Apple Developer Team.  This build does not
need that entitlement for ordinary local file access.
