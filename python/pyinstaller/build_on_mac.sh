#!/bin/bash

# A shell command to build a single file executable of the 
# analyzer's app.

set -e
set -o xtrace

if [[ $OSTYPE != darwin* ]]; then
  echo "Unexpected OSTYPE for linux: [$OSTYPE]"
  exit 1
fi

#pip install -U pyinstaller

rm -rf _dist
rm -rf _build
rm -rf _spec

mkdir _dist
mkdir _build
mkdir _spec

/Library/Frameworks/Python.framework/Versions/3.11/bin/pyinstaller ../analyzer/analyzer.py \
  --collect-submodules dbus_fast \
  --paths ".." \
  --clean \
  --onefile \
  --distpath _dist  \
  --workpath _build \
  --specpath _spec 

ls -al _dist

cp _dist/analyzer ../../release/mac/analyzer


