#!/usr/bin/bash -x

# A shell command to build a single file executable of the 
# analyzer's app.

#pip install -U pyinstaller

rm -rf _dist
rm -rf _build
rm -rf _spec

mkdir _dist
mkdir _build
mkdir _spec

pyinstaller ../analyzer/analyzer.py \
  --add-binary /home/user/.local/lib/python3.10/site-packages/dbus_fast/_private/marshaller.cpython-310-x86_64-linux-gnu.so:dbus_fast._private.marshaller \
  --paths ".." \
  --clean \
  --onefile \
  --distpath _dist  \
  --workpath _build \
  --specpath _spec 

ls -al _dist


