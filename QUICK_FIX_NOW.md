# Quick Fix for Current Build Error

## Immediate Fix (Run in WSL)

Copy and paste these commands into your WSL terminal:

```bash
cd /mnt/c/Users/pvida/Documents/GitHub/dogecoin/depends

# Remove problematic Qt files with sudo
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtCore/QObject
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtCore/qobject.h
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtCore/qstring.h
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtWidgets/QMessageBox
sudo rm -rf work/build/x86_64-w64-mingw32/qt/5.7.1-905c91ecf1c/qtbase/include/QtWidgets/qmessagebox.h

# Or just remove the entire Qt build directory (easier)
sudo rm -rf work/build/x86_64-w64-mingw32/qt

# Fix permissions
sudo chmod -R u+w work

# Clean staging directory
sudo rm -rf work/staging/x86_64-w64-mingw32/qt

# Continue build
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
HOST=x86_64-w64-mingw32 make
```

## Alternative: Use the Fix Script

```bash
cd /mnt/c/Users/pvida/Documents/GitHub/dogecoin
bash fix-permission-errors.sh

# Then continue build
cd depends
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
HOST=x86_64-w64-mingw32 make
```

## Best Solution: Build on WSL Native Filesystem

To avoid these issues in the future, build on WSL native filesystem:

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

# Clean Qt build
rm -rf depends/work/build/x86_64-w64-mingw32/qt
rm -rf depends/work/staging/x86_64-w64-mingw32/qt

# Build
cd depends
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'
HOST=x86_64-w64-mingw32 make
cd ..

# Copy executables back to Windows
cp src/qt/dogecoin-qt.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/qt/
cp src/dogecoind.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
cp src/dogecoin-cli.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
cp src/dogecoin-tx.exe /mnt/c/Users/pvida/Documents/GitHub/dogecoin/src/
```

This will be faster and won't have permission issues!



