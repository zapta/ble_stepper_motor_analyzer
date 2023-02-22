#!/bin/bash -x

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
  --paths ".." \
  --clean \
  --onefile \
  --distpath _dist  \
  --workpath _build \
  --specpath _spec 

ls -al _dist


