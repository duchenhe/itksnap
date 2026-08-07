#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_APP="$ROOT_DIR/build/ITK-SNAP-Patched.app"
DIST_DIR="$ROOT_DIR/build/distribution/macos-arm64"
APP_NAME="ITK-SNAP-Patched.app"
APP_BUNDLE="$DIST_DIR/$APP_NAME"
APP_BINARY="$APP_BUNDLE/Contents/MacOS/ITK-SNAP-Patched"
DMG_STAGING="$DIST_DIR/dmg-staging"
VERSION="4.4.0"

SIGN_IDENTITY="${MACOS_SIGN_IDENTITY:-}"
NOTARY_PROFILE="${MACOS_NOTARY_PROFILE:-}"
ENTITLEMENTS="${MACOS_ENTITLEMENTS:-}"

if [[ ! -d "$SOURCE_APP" ]]; then
  echo "Missing source application: $SOURCE_APP" >&2
  exit 1
fi

mkdir -p "$DIST_DIR"
if [[ -e "$APP_BUNDLE" || -e "$DMG_STAGING" ]]; then
  echo "Distribution staging already exists under $DIST_DIR." >&2
  echo "Move it aside before rebuilding so an older signed artifact is not overwritten." >&2
  exit 1
fi

ditto "$SOURCE_APP" "$APP_BUNDLE"
/usr/libexec/PlistBuddy -c 'Set :CFBundleExecutable ITK-SNAP-Patched' "$APP_BUNDLE/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier org.itksnap.patched' "$APP_BUNDLE/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Set :CFBundleName ITK-SNAP Patched' "$APP_BUNDLE/Contents/Info.plist"
/usr/libexec/PlistBuddy -c 'Set :CFBundleDisplayName ITK-SNAP Patched' "$APP_BUNDLE/Contents/Info.plist"
if /usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' "$APP_BUNDLE/Contents/Info.plist" >/dev/null 2>&1; then
  /usr/libexec/PlistBuddy -c 'Set :LSMinimumSystemVersion 26.0' "$APP_BUNDLE/Contents/Info.plist"
else
  /usr/libexec/PlistBuddy -c 'Add :LSMinimumSystemVersion string 26.0' "$APP_BUNDLE/Contents/Info.plist"
fi

"$ROOT_DIR/script/fixup_macos_bundle.py" "$APP_BUNDLE"

sign_loose_code() {
  local identity="$1"
  local timestamp_flag="$2"
  local args=(--force)
  if [[ "$identity" != '-' ]]; then
    args+=(--options runtime)
  fi
  if [[ -n "$timestamp_flag" ]]; then
    args+=("$timestamp_flag")
  fi
  args+=(--sign "$identity")
  while IFS= read -r -d '' file; do
    if file "$file" | rg -q 'Mach-O' && [[ "$file" != *'.framework/'* ]]; then
      /usr/bin/codesign "${args[@]}" "$file"
    fi
  done < <(find "$APP_BUNDLE/Contents" -type f -print0)
}

sign_frameworks() {
  local identity="$1"
  local timestamp_flag="$2"
  local args=(--force)
  if [[ "$identity" != '-' ]]; then
    args+=(--options runtime)
  fi
  if [[ -n "$timestamp_flag" ]]; then
    args+=("$timestamp_flag")
  fi
  args+=(--sign "$identity")
  while IFS= read -r framework; do
    /usr/bin/codesign "${args[@]}" "$framework"
  done < <(find "$APP_BUNDLE/Contents/Frameworks" -type d -name '*.framework' | sort)
}

sign_app() {
  local identity="$1"
  local timestamp_flag="$2"
  local args=(--force)
  if [[ "$identity" != '-' ]]; then
    args+=(--options runtime)
  fi
  if [[ -n "$timestamp_flag" ]]; then
    args+=("$timestamp_flag")
  fi
  if [[ -n "$ENTITLEMENTS" ]]; then
    args+=(--entitlements "$ENTITLEMENTS")
  fi
  args+=(--sign "$identity" "$APP_BUNDLE")
  /usr/bin/codesign "${args[@]}"
}

if [[ -n "$SIGN_IDENTITY" ]]; then
  if ! security find-identity -v -p codesigning | rg -Fq "$SIGN_IDENTITY"; then
    echo "Developer ID identity is not available: $SIGN_IDENTITY" >&2
    exit 1
  fi
  sign_loose_code "$SIGN_IDENTITY" --timestamp
  sign_frameworks "$SIGN_IDENTITY" --timestamp
  sign_app "$SIGN_IDENTITY" --timestamp
  DMG_PATH="$DIST_DIR/ITK-SNAP-Patched-${VERSION}-macOS-arm64.dmg"
else
  echo "No MACOS_SIGN_IDENTITY supplied; creating an explicitly unnotarized test artifact."
  sign_loose_code - ""
  sign_frameworks - ""
  sign_app - ""
  DMG_PATH="$DIST_DIR/ITK-SNAP-Patched-${VERSION}-macOS-arm64-UNNOTARIZED.dmg"
fi

/usr/bin/codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"

mkdir -p "$DMG_STAGING"
ditto "$APP_BUNDLE" "$DMG_STAGING/$APP_NAME"
ln -s /Applications "$DMG_STAGING/Applications"
cp "$ROOT_DIR/COPYING" "$DMG_STAGING/COPYING-GPLv3.txt"

hdiutil create \
  -volname 'ITK-SNAP Patched' \
  -srcfolder "$DMG_STAGING" \
  -format UDZO \
  "$DMG_PATH"

if [[ -n "$SIGN_IDENTITY" ]]; then
  /usr/bin/codesign --force --timestamp --sign "$SIGN_IDENTITY" "$DMG_PATH"
  if [[ -z "$NOTARY_PROFILE" ]]; then
    echo "DMG is Developer ID signed but MACOS_NOTARY_PROFILE is missing." >&2
    exit 1
  fi
  xcrun notarytool submit "$DMG_PATH" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$DMG_PATH"
  xcrun stapler validate "$DMG_PATH"
  /usr/sbin/spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG_PATH"
fi

shasum -a 256 "$DMG_PATH" > "$DMG_PATH.sha256"
echo "Created: $DMG_PATH"
