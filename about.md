# SmoothDash

Run Geometry Dash at high FPS without the stutter. Defaults are tuned for budget laptops with integrated graphics (Ryzen 5 7520U / Radeon 610M class).

## What it does
- **Unlock FPS** – sets your own frame cap (default 120, or 0 for uncapped).
- **High-precision timer** – asks Windows for a 1 ms timer so frames are evenly spaced. This is the main micro-stutter fix.
- **Disable power throttling** – keeps Windows from slowing GD down to save battery.
- **Above-normal priority** – keeps background apps from stealing CPU time.
- **Force Low Detail Mode** – turns on LDM in every level that has it. Biggest FPS gain on integrated graphics.
- **Particle amount** – trims particles (default 50%).
- **Disable shake** (off by default) – removes screen shake.

## Tips for laptops with integrated graphics
- Turn **VSync off** in GD's graphics settings.
- Set GD's **Texture Quality to Medium**. The iGPU shares system RAM, so smaller textures load faster and stutter less.
- **Plug in your charger** and set Windows power mode to **Best performance**. On battery the CPU and GPU clock way down.
- In **AMD Software: Adrenalin**, make sure **Radeon Chill** is off for GD.
- Close Chrome/Discord while playing if you can; they compete for the same shared memory.
