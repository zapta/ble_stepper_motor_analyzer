#!/bin/bash -x

# A shell command to build a single file executable of the 
# analyzer's app.

#pip install -U pyinstaller

rm -rf .pyinstaller.dist
rm -rf .pyinstaller.build
rm -rf .pyinstaller.spec

pyinstaller analyzer.py \
  --clean \
  --onefile \
  --distpath .pyinstaller.dist  \
  --workpath .pyinstaller.build \
  --specpath .pyinstaller.spec 

ls -al ./.pyinstaller.dist


