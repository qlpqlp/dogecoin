# Building Dogecoin Core on Windows 11

This guide provides step-by-step instructions for building Dogecoin Core on Windows 11 that will run natively on Windows 11.

## Overview

Dogecoin Core is built using **cross-compilation** from Linux to Windows. The recommended approach on Windows 11 is to use **Windows Subsystem for Linux 2 (WSL2)**, which provides the best compatibility and performance.

## Prerequisites

### 1. Windows 11 Requirements

- **Windows 11** (64-bit) - WSL2 is included and recommended
- At least **8GB RAM** (16GB recommended)
- At least **50GB free disk space** for the build process
- **Administrator access** for initial setup

### 2. Enable WSL2

Windows 11 comes with WSL2 support built-in. To enable it:

1. **Open PowerShell as Administrator** (Right-click Start → Windows PowerShell (Admin))

2. **Enable WSL and Virtual Machine Platform:**
   ```powershell
   wsl --install
   ```
   This will:
   - Enable the required Windows features
   - Install WSL2 and a default Linux distribution (Ubuntu)
   - Set WSL2 as the default version

3. **Restart your computer** when prompted

4. **Verify WSL version:**
   ```powershell
   wsl --status
   wsl -l -v
   ```
   You should see WSL2 listed. If you see WSL1, convert it:
   ```powershell
   wsl --set-version Ubuntu 2
   ```

### 3. Install Ubuntu (if not already installed)

1. Open **Microsoft Store**
2. Search for **"Ubuntu 22.04 LTS"** or **"Ubuntu 20.04 LTS"**
3. Click **Install**
4. Launch Ubuntu from Start Menu
5. Create a **UNIX username and password** (this is separate from your Windows account)

### 4. Update Ubuntu

Open WSL (type `wsl` in PowerShell or launch Ubuntu from Start Menu):

```bash
sudo apt update
sudo apt upgrade -y
```

## Building Dogecoin Core

### Option 1: Build in WSL Native Filesystem (Recommended)

This method is faster and more reliable because it avoids Windows filesystem permission issues.

#### Step 1: Copy Source to WSL Native Filesystem

```bash
# In WSL terminal
cd ~
cp -r /mnt/c/Users/pvida/Documents/GitHub/dogecoin ~/dogecoin
cd ~/dogecoin
```

#### Step 2: Install Build Dependencies

```bash
# Install general build tools
sudo apt-get update
sudo apt-get install -y build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git

# Install cross-compilation toolchain for Windows 64-bit
sudo apt-get install -y g++-mingw-w64-x86-64

# Configure mingw-w64 to use posix threading (required)
sudo update-alternatives --config x86_64-w64-mingw32-g++
# Choose option with "posix" (usually option 1)
```

#### Step 3: Fix Line Endings (Windows CRLF to Unix LF)

```bash
cd ~/dogecoin/depends
# Install dos2unix if needed
sudo apt-get install -y dos2unix
dos2unix config.guess config.sub
chmod +x config.guess config.sub
cd ..
```

#### Step 4: Build Dependencies (30-60 minutes)

```bash
cd ~/dogecoin/depends
make HOST=x86_64-w64-mingw32
cd ..
```

This builds all required libraries (Qt, Boost, Berkeley DB, etc.) for Windows.

#### Step 5: Generate Configure Script

```bash
cd ~/dogecoin
./autogen.sh
```

#### Step 6: Configure Build

```bash
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
```

#### Step 7: Build Dogecoin Core (30-60 minutes)

```bash
make
```

**Note:** Avoid using `-j` flag (parallel builds) as it can cause compilation errors.

#### Step 8: Copy Executable to Windows

After build completes, copy the executable to your Windows directory:

```bash
# Copy to Windows filesystem
cp src/qt/dogecoin-qt.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/qt/
cp src/dogecoind.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
cp src/dogecoin-cli.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
cp src/dogecoin-tx.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
```

### Option 2: Build on Windows Filesystem (Alternative)

If you prefer to build directly on the Windows filesystem:

#### Step 1: Open WSL Terminal

Type `wsl` in PowerShell or launch Ubuntu from Start Menu.

#### Step 2: Navigate to Project

```bash
cd /mnt/c/Users/pvida/Documents/GitHub/dogecoin
```

#### Step 3: Clean PATH (Important!)

Windows paths can cause build issues. Clean them:

```bash
export PATH=$(echo "$PATH" | tr ':' '\n' | grep -v '^/mnt' | grep -v '(' | grep -v ')' | tr '\n' ':' | sed 's/:$//')
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH"
```

#### Step 4: Install Dependencies

Same as Option 1, Step 2.

#### Step 5: Build

Follow steps 3-7 from Option 1, but run from `/mnt/c/Users/pvida/Documents/GitHub/dogecoin`.

### Option 3: Use Automated Build Script

If you have the build scripts available:

#### From Windows (PowerShell):

```powershell
cd C:\Users\pvida\Documents\GitHub\dogecoin
.\build-pos.bat
```

#### From WSL:

```bash
cd /mnt/c/Users/pvida/Documents/GitHub/dogecoin
bash build-pos.sh
```

## Running the Build

After successful build, the executables will be located at:

- **GUI Application:** `src/qt/dogecoin-qt.exe`
- **Daemon:** `src/dogecoind.exe`
- **CLI Tool:** `src/dogecoin-cli.exe`
- **Transaction Tool:** `src/dogecoin-tx.exe`

### To Run on Windows 11:

1. Navigate to the `src/qt/` directory in File Explorer
2. Double-click `dogecoin-qt.exe` to launch the GUI
3. Or run from Command Prompt/PowerShell:
   ```powershell
   cd C:\Users\pvida\Documents\GitHub\dogecoin\src\qt
   .\dogecoin-qt.exe
   ```

## Troubleshooting

### Issue: "g++-mingw-w64-x86-64 not found"

**Solution:**
```bash
sudo apt-get update
sudo apt-get install -y g++-mingw-w64-x86-64
```

### Issue: "posix threading model not found"

**Solution:**
```bash
sudo update-alternatives --config x86_64-w64-mingw32-g++
# Select the option with "posix"
```

### Issue: Permission Denied Errors

**Solution:** Use WSL native filesystem (Option 1) instead of Windows filesystem.

### Issue: Build Fails with Qt5Sql Error

**Solution:**
```bash
sudo apt-get install -y qtbase5-dev
# Then rebuild from Step 4
```

### Issue: "config.guess: Permission denied"

**Solution:**
```bash
cd depends
chmod +x config.guess config.sub
dos2unix config.guess config.sub
```

### Issue: WSL1 Detected (Should use WSL2)

**Solution:**
```powershell
# In PowerShell (Admin)
wsl --set-version Ubuntu 2
wsl --set-default-version 2
```

### Issue: Out of Disk Space

**Solution:** Clean up build artifacts:
```bash
cd ~/dogecoin  # or your build directory
make clean
# Or remove depends build directory:
rm -rf depends/work depends/built
```

## Build Time Estimates

- **Dependencies:** 30-60 minutes (first time only)
- **Configure:** 2-5 minutes
- **Dogecoin Core:** 30-60 minutes
- **Total:** 1-2 hours (depending on CPU/RAM)

## System Requirements

- **CPU:** 4+ cores recommended
- **RAM:** 8GB minimum, 16GB recommended
- **Disk:** 50GB+ free space
- **OS:** Windows 11 (64-bit)

## Additional Notes

### Windows 11 Specific Advantages

- **WSL2 is default** - Better performance than WSL1
- **Better file system performance** - WSL2 uses a virtualized filesystem
- **Full compatibility** - Windows 11 has full WSL2 support

### Performance Tips

1. **Use WSL native filesystem** - Faster than Windows filesystem
2. **Close unnecessary applications** - Free up RAM during build
3. **Disable antivirus scanning** - Can slow down build (temporarily)
4. **Use SSD** - Faster than HDD

### After First Build

Subsequent builds will be faster if you:
- Don't clean the `depends/` directory
- Only rebuild changed files using `make`

## Verification

After building, verify the executable:

```powershell
# In PowerShell
cd C:\Users\pvida\Documents\GitHub\dogecoin\src\qt
.\dogecoin-qt.exe --version
```

You should see version information indicating a successful build.

## Next Steps

1. **Run Dogecoin Core** - Launch `dogecoin-qt.exe`
2. **Sync blockchain** - Wait for initial blockchain download
3. **Configure** - Set up your wallet and preferences
4. **Test** - Verify the Point of Sale feature if enabled

## Getting Help

If you encounter issues:

1. Check the build logs for specific error messages
2. Review the troubleshooting section above
3. Check existing documentation in `doc/build-windows.md`
4. Search for similar issues in the Dogecoin repository

## Summary

The build process for Windows 11 is straightforward:

1. ✅ Enable WSL2
2. ✅ Install Ubuntu
3. ✅ Install build dependencies
4. ✅ Build dependencies (30-60 min)
5. ✅ Configure and build Dogecoin Core (30-60 min)
6. ✅ Run `dogecoin-qt.exe` on Windows 11

The resulting executables are native Windows applications that will run perfectly on Windows 11.



