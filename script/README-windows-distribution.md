# ITK-SNAP Patched Windows distribution

The patched Windows build is produced by
`.github/workflows/build-windows-patched.yml` on a GitHub-hosted
`windows-2022` runner.  A public fork uses a standard runner and does not
require paid GitHub Actions minutes.

## Create the fork and start the first build

1. Open <https://github.com/pyushkevich/itksnap/fork> while signed in to the
   `duchenhe` GitHub account.
2. Keep the repository public, retain the name `itksnap`, and create the fork.
3. Open the fork's **Actions** tab and select **I understand my workflows, go
   ahead and enable them** if GitHub displays that prompt.
4. Push the local `codex/fix-oblique-orientation` branch to the fork.  That
   branch name automatically triggers only the **Build Windows Patched**
   workflow.  The upstream four-platform workflow is restricted to `master`.

The workflow builds ITK and VTK, builds the patched application, runs
`ImageCoordinateGeometryTest`, packages an unsigned NSIS installer, calculates
its SHA-256 digest, and uploads both files as the artifact
`ITK-SNAP-Patched-4.4.0-Windows-x64`.

## Download and install

Open the successful workflow run and download the artifact from its
**Artifacts** section.  Extract the downloaded ZIP before running:

```text
ITK-SNAP-Patched-4.4.0-Windows-x64.exe
```

The installer and application are intentionally separate from upstream
ITK-SNAP:

- executable: `ITK-SNAP-Patched.exe`
- Start menu display name: `ITK-SNAP Patched`
- install directory: `C:\Program Files\ITK-SNAP Patched 4.4`
- uninstall registry key: `ITK-SNAP Patched 4.4`

Because this test release is unsigned, Windows SmartScreen is expected to show
**Windows protected your PC**.  Verify the checksum first, then choose **More
info** and **Run anyway**.  In PowerShell, calculate the downloaded file's
digest with:

```powershell
Get-FileHash .\ITK-SNAP-Patched-4.4.0-Windows-x64.exe -Algorithm SHA256
```

Compare that value with the uploaded `.sha256` file.  Enterprise-managed PCs
may prohibit running unsigned software even after the checksum is verified; in
that case the collaborator needs an administrator or IT exception.
