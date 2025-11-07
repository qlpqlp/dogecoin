# Quick Start Guide - Building Dogecoin Core on Windows 11

## Fastest Method (5 Commands)

1. **Open WSL:**
   ```powershell
   wsl
   ```

2. **Copy to WSL native filesystem (recommended):**
   ```bash
   cd ~
   cp -r /mnt/c/Users/pvida/Documents/GitHub/dogecoin ~/dogecoin
   cd ~/dogecoin
   ```

3. **Install dependencies:**
   ```bash
   sudo apt-get update && sudo apt-get install -y build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git g++-mingw-w64-x86-64
   sudo update-alternatives --config x86_64-w64-mingw32-g++
   # Choose option 1 (posix)
   ```

4. **Run automated build script:**
   ```bash
   bash build-windows11.sh
   ```

5. **Copy executable to Windows:**
   ```bash
   cp src/qt/dogecoin-qt.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/qt/
   ```

## Alternative: Use Windows Batch File

1. **Double-click:** `build-windows11.bat`
2. **Wait 1-2 hours** for the build to complete
3. **Run:** `src\qt\dogecoin-qt.exe`

## Prerequisites Checklist

- [ ] Windows 11 (64-bit)
- [ ] WSL2 enabled (`wsl --install`)
- [ ] Ubuntu installed (from Microsoft Store)
- [ ] At least 8GB RAM free
- [ ] At least 50GB free disk space

## Build Time

- **Dependencies:** 30-60 minutes (first time only)
- **Dogecoin Core:** 30-60 minutes
- **Total:** 1-2 hours

## After Build

The executable `dogecoin-qt.exe` will run natively on Windows 11!

## Troubleshooting

**"WSL not found"** → Run `wsl --install` in PowerShell (Admin)

**"g++-mingw-w64-x86-64 not found"** → Run: `sudo apt-get install -y g++-mingw-w64-x86-64`

**"posix not found"** → Run: `sudo update-alternatives --config x86_64-w64-mingw32-g++` and choose option 1

**Build fails** → Use WSL native filesystem (copy to ~/dogecoin) instead of Windows filesystem

## Full Documentation

See `BUILD_WINDOWS11.md` for complete instructions.



