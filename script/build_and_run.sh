#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/app"
APP_BUNDLE="$ROOT_DIR/build/ITK-SNAP-Patched-Dev.app"
APP_BINARY="$APP_BUNDLE/Contents/MacOS/ITK-SNAP-Patched"
CMAKE_BIN="$ROOT_DIR/build/tools/cmake-3.30.9-macos-universal/CMake.app/Contents/bin/cmake"
CTEST_BIN="$ROOT_DIR/build/tools/cmake-3.30.9-macos-universal/CMake.app/Contents/bin/ctest"
ITK_DIR="$ROOT_DIR/build/itk-install/lib/cmake/ITK-5.4"

MODE="run"
IMAGE_PATH=""
if [[ $# -gt 0 ]]; then
  case "$1" in
    run|--debug|debug|--logs|logs|--telemetry|telemetry|--verify|verify)
      MODE="$1"
      shift
      ;;
    *)
      IMAGE_PATH="$1"
      shift
      ;;
  esac
fi
if [[ $# -gt 0 ]]; then
  IMAGE_PATH="$1"
  shift
fi
if [[ $# -gt 0 ]]; then
  echo "usage: $0 [run|--debug|--logs|--telemetry|--verify] [image.nii.gz]" >&2
  exit 2
fi

if [[ ! -x "$CMAKE_BIN" || ! -d "$ITK_DIR" ]]; then
  echo "The project-local CMake or legacy-enabled ITK build is missing under $ROOT_DIR/build." >&2
  echo "Restore the prepared build dependencies before running this script." >&2
  exit 1
fi

pkill -x ITK-SNAP-Patched >/dev/null 2>&1 || true

"$CMAKE_BIN" -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$ROOT_DIR/build/itk-install;/opt/homebrew/opt/qt;/opt/homebrew/opt/libssh;/opt/homebrew/opt/vtk;/opt/homebrew/opt/hdf5" \
  -DITK_DIR="$ITK_DIR" \
  -DVTK_DIR=/opt/homebrew/opt/vtk/lib/cmake/vtk-9.6 \
  -DHDF5_DIR=/opt/homebrew/lib/cmake/hdf5 \
  -DSNAP_USE_VTK_RENDERING_EXTERNAL=OFF \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.6
"$CMAKE_BIN" --build "$BUILD_DIR" --target ITK-SNAP image_coordinate_geometry_test color_map_preset_test --parallel 8
"$CTEST_BIN" --test-dir "$BUILD_DIR" --output-on-failure -R '^(ImageCoordinateGeometryTest|ColorMapPresetTest)$'

if [[ ! -d "$APP_BUNDLE" ]]; then
  mkdir -p "$APP_BUNDLE/Contents/MacOS" "$APP_BUNDLE/Contents/Resources"
  cp "$BUILD_DIR/Info.plist" "$APP_BUNDLE/Contents/Info.plist"
  cp "$ROOT_DIR/Utilities/MacOS/BundleResources/itksnap.icns" "$APP_BUNDLE/Contents/Resources/ITK-SNAP.icns"
  /usr/libexec/PlistBuddy -c 'Set :CFBundleExecutable ITK-SNAP-Patched' "$APP_BUNDLE/Contents/Info.plist"
  /usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier org.itksnap.patched.dev' "$APP_BUNDLE/Contents/Info.plist"
  /usr/libexec/PlistBuddy -c 'Set :CFBundleName ITK-SNAP Patched Dev' "$APP_BUNDLE/Contents/Info.plist"
  /usr/libexec/PlistBuddy -c 'Set :CFBundleDisplayName ITK-SNAP Patched Dev' "$APP_BUNDLE/Contents/Info.plist"
fi

cp "$BUILD_DIR/ITK-SNAP" "$APP_BINARY"
cp "$ROOT_DIR/script/qt-homebrew.conf" "$APP_BUNDLE/Contents/Resources/qt.conf"
chmod +x "$APP_BINARY"
/usr/bin/codesign --force --deep --sign - "$APP_BUNDLE" >/dev/null

LAUNCH_ARGS=()
if [[ -n "$IMAGE_PATH" ]]; then
  LAUNCH_ARGS=(-g "$IMAGE_PATH")
fi

open_app() {
  if [[ ${#LAUNCH_ARGS[@]} -gt 0 ]]; then
    /usr/bin/open -n "$APP_BUNDLE" --args "${LAUNCH_ARGS[@]}"
  else
    /usr/bin/open -n "$APP_BUNDLE"
  fi
}

case "$MODE" in
  run)
    open_app
    ;;
  --debug|debug)
    lldb -- "$APP_BINARY" "${LAUNCH_ARGS[@]}"
    ;;
  --logs|logs)
    open_app
    /usr/bin/log stream --info --style compact --predicate 'process == "ITK-SNAP-Patched"'
    ;;
  --telemetry|telemetry)
    open_app
    /usr/bin/log stream --info --style compact --predicate 'subsystem == "org.itksnap.patched"'
    ;;
  --verify|verify)
    open_app
    sleep 2
    pgrep -f "$APP_BINARY" >/dev/null
    ;;
  *)
    echo "usage: $0 [run|--debug|--logs|--telemetry|--verify] [image.nii.gz]" >&2
    exit 2
    ;;
esac
