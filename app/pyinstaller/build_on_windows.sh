#!/bin/bash

# A shell command to build a single file executable of the 
# analyzer's app.

set -e
set -o xtrace

if [[ $OSTYPE != "win32" ]]; then
  echo "Unexpected OSTYPE for windows: [$OSTYPE]"
  exit 1
fi

#pip install -U pyinstaller

rm -rf _dist
rm -rf _build
rm -rf _spec

mkdir _dist
mkdir _build
mkdir _spec

pyinstaller ../analyzer/analyzer.py \
  --paths ".." \
  --clean \
  --onefile \
  --distpath _dist  \
  --workpath _build \
  --specpath _spec 

ls -al _dist

cp _dist/analyzer.exe ../../release/linux/analyzer.exe



