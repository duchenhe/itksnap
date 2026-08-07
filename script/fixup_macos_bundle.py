#!/usr/bin/env python3
"""Make a macOS app bundle independent of Homebrew absolute paths."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys


SYSTEM_PREFIXES = ("/System/Library/", "/usr/lib/")


def run(*args: str, capture: bool = False) -> str:
    result = subprocess.run(
        args,
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )
    return result.stdout if capture else ""


def is_macho(path: Path) -> bool:
    if not path.is_file():
        return False
    result = subprocess.run(
        ("file", str(path)), text=True, capture_output=True, check=True
    )
    return "Mach-O" in result.stdout


def macho_files(contents: Path) -> list[Path]:
    # Frameworks contain several symlinks to the same binary.  Editing the
    # target through every alias would attempt to rewrite one load command
    # more than once, so operate on each resolved file exactly once.
    result: list[Path] = []
    seen: set[Path] = set()
    for path in contents.rglob("*"):
        if not is_macho(path):
            continue
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        result.append(resolved)
    return sorted(result)


def dependencies(path: Path) -> list[str]:
    output = run("otool", "-L", str(path), capture=True)
    return [line.strip().split(" ", 1)[0] for line in output.splitlines()[1:]]


def install_id(path: Path) -> str | None:
    output = run("otool", "-D", str(path), capture=True)
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    return lines[1] if len(lines) > 1 else None


def rpaths(path: Path) -> list[str]:
    output = run("otool", "-l", str(path), capture=True)
    result: list[str] = []
    expect_path = False
    for line in output.splitlines():
        fields = line.strip().split()
        if fields == ["cmd", "LC_RPATH"]:
            expect_path = True
        elif expect_path and len(fields) >= 2 and fields[0] == "path":
            result.append(fields[1])
            expect_path = False
    return result


def framework_parts(dependency: str) -> tuple[str, str] | None:
    marker = ".framework/"
    if marker not in dependency:
        return None
    before, after = dependency.split(marker, 1)
    name = Path(before).name + ".framework"
    return name, after


def bundled_target(frameworks: Path, dependency: str) -> Path:
    framework = framework_parts(dependency)
    if framework:
        name, suffix = framework
        return frameworks / name / suffix
    return frameworks / Path(dependency).name


def copy_dependency(frameworks: Path, dependency: str) -> bool:
    framework = framework_parts(dependency)
    if framework:
        name, _ = framework
        destination = frameworks / name
        if destination.exists():
            return False
        source_root = Path(dependency.split(".framework/", 1)[0] + ".framework")
        run("ditto", str(source_root), str(destination))
        return True

    destination = frameworks / Path(dependency).name
    if destination.exists():
        return False
    shutil.copy2(Path(dependency).resolve(), destination)
    return True


def loader_reference(binary: Path, target: Path) -> str:
    relative = os.path.relpath(target, binary.parent)
    return "@loader_path/" + relative


def bundled_install_id(binary: Path, frameworks: Path) -> str:
    try:
        relative = binary.relative_to(frameworks)
    except ValueError:
        relative = Path(binary.name)
    return "@rpath/" + str(relative)


def fix_bundle(app: Path) -> None:
    contents = app / "Contents"
    frameworks = contents / "Frameworks"
    if not (contents / "MacOS").is_dir():
        raise RuntimeError(f"Not an application bundle: {app}")
    frameworks.mkdir(parents=True, exist_ok=True)

    # Complete the dependency closure before rewriting paths. Newly copied
    # libraries may themselves introduce additional Homebrew dependencies.
    while True:
        copied = False
        for binary in macho_files(contents):
            for dependency in dependencies(binary):
                if dependency.startswith("/") and not dependency.startswith(SYSTEM_PREFIXES):
                    copied = copy_dependency(frameworks, dependency) or copied
        if not copied:
            break

    for binary in macho_files(contents):
        identity = install_id(binary)
        if identity and identity.startswith("/") and not identity.startswith(SYSTEM_PREFIXES):
            run(
                "install_name_tool",
                "-id",
                bundled_install_id(binary, frameworks),
                str(binary),
            )

        for dependency in dependencies(binary):
            if dependency.startswith("/") and not dependency.startswith(SYSTEM_PREFIXES):
                target = bundled_target(frameworks, dependency)
                if not target.exists():
                    raise RuntimeError(f"Missing bundled dependency for {dependency}")
                run(
                    "install_name_tool",
                    "-change",
                    dependency,
                    loader_reference(binary, target),
                    str(binary),
                )

        for rpath in rpaths(binary):
            if rpath.startswith(("/opt/homebrew/", "/Users/")):
                run("install_name_tool", "-delete_rpath", rpath, str(binary))

    leaks: list[str] = []
    for binary in macho_files(contents):
        for dependency in dependencies(binary):
            if dependency.startswith("/") and not dependency.startswith(SYSTEM_PREFIXES):
                leaks.append(f"{binary}: {dependency}")
        for rpath in rpaths(binary):
            if rpath.startswith(("/opt/homebrew/", "/Users/")):
                leaks.append(f"{binary}: LC_RPATH {rpath}")
    if leaks:
        raise RuntimeError("External paths remain:\n" + "\n".join(leaks))


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} APP_BUNDLE", file=sys.stderr)
        return 2
    app = Path(sys.argv[1]).resolve()
    fix_bundle(app)
    print(f"Dependency fixup complete: {app}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
