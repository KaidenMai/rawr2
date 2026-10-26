# SmoothDash – how to build

## Easiest: GitHub (no setup on your PC)
1. Make a new repo on GitHub and upload everything in this folder (keep the `.github` folder!).
2. Go to the **Actions** tab. A build starts automatically (or click **Build Geode Mod → Run workflow**).
3. When it finishes (~5–10 min), open the run and download the **SmoothDash** artifact. Inside is `yourname.smoothdash.geode`.
4. Put that file in `Geometry Dash/geode/mods/` and restart GD.

## On your own PC
1. Install Visual Studio (with "Desktop development with C++"), CMake, and the Geode CLI – see https://docs.geode-sdk.org/getting-started/
2. Run `geode sdk install` and `geode sdk install-binaries`.
3. In this folder run `geode build`. The `.geode` file ends up in `build/`.

## Before publishing
Change `"id"` and `"developer"` in `mod.json` from `yourname` / `YourName` to your own name.
