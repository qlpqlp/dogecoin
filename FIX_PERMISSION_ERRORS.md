# Fix Permission Errors During Build

## Current Problem

You're seeing these errors:
```
rm: cannot remove '...': Permission denied
mkdir: cannot create directory 'qtbase': File exists
```

This happens when building on Windows filesystem (`/mnt/c/`) because Windows doesn't preserve Unix permissions properly.

## Quick Fix (Choose One)

### Option 1: Use Sudo to Clean (Fastest Fix)

**In WSL terminal:**

```bash
cd /mnt/c/Users/pvida/Documents/GitHub/dogecoin/depends

# Use sudo to remove problematic Qt files
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtCore/QObject
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtCore/qobject.h
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtCore/qstring.h
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtWidgets/QMessageBox
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtWidgets/qmessagebox.h

# Or clean the entire Qt build directory
sudo rm -rf work/build/x86_64-w64-mingw32/qt

# Fix permissions on work directory
sudo chmod -R u+w work

# Clean staging directory too
sudo rm -rf work/staging/x86_64-w64-mingw32/qt

# Continue build
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
HOST=x86_64-w64-mingw32 make
```

### Option 2: Fix from Windows (PowerShell as Admin)

**Run PowerShell as Administrator:**

```powershell
cd C:\Users\pvida\Documents\GitHub\dogecoin

# Take ownership of the depends directory
takeown /F "depends\work" /R /D Y
icacls "depends\work" /grant "${env:USERNAME}:F" /T /C /Q

# Remove problematic Qt directories
Remove-Item -Path "depends\work\build\x86_64-w64-mingw32\qt" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path "depends\work\staging\x86_64-w64-mingw32\qt" -Recurse -Force -ErrorAction SilentlyContinue
```

**Then in WSL, continue build:**

```bash
cd /mnt/c/Users/pvida/Documents/GitHub/dogecoin/depends
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
HOST=x86_64-w64-mingw32 make
```

### Option 3: Build in WSL Native Filesystem (Best Solution)

**This avoids ALL permission issues:**

```bash
# Copy to WSL native filesystem
cd ~
cp -r /mnt/c/Users/pvida/Documents/GitHub/dogecoin ~/dogecoin
cd ~/dogecoin

# Fix line endings
cd depends
sed -i 's/\r$//' config.guess config.sub
chmod +x config.guess config.sub
cd ..

# Clean and rebuild
cd depends
rm -rf work/build/x86_64-w64-mingw32/qt work/staging/x86_64-w64-mingw32/qt

# Build
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
HOST=x86_64-w64-mingw32 make
cd ..

# After build completes, copy executables back
cp src/qt/dogecoin-qt.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/qt/
cp src/dogecoind.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
cp src/dogecoin-cli.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
cp src/dogecoin-tx.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
```

## Why This Happens

- Windows filesystem (`/mnt/c/`) doesn't preserve Unix file permissions
- Some files get locked by Windows processes
- Build scripts try to `rm` files but Windows filesystem denies access
- This is a known limitation of WSL when building on Windows filesystem

## Recommendation

**Use Option 3 (WSL native filesystem)** - it's the most reliable solution:
- ✅ No permission issues
- ✅ Better performance (faster I/O)
- ✅ No case sensitivity problems
- ✅ No path length limitations

The build will be faster and more reliable on WSL native filesystem.



