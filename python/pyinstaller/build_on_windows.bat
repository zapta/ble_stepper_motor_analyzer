:: A Windows batch file to build a single file executable of the 
:: analyzer's app.
::
:: See this bug regarding an error in pyqtgraph and the workaround of swapping
:: two lines:
:: https://github.com/pyinstaller/pyinstaller/issues/7991#issuecomment-1752032919

::set -e
::set -o xtrace

echo Hello world

rd /s/q _dist
rd /s/q _build
rd /s/q _spec


mkdir _dist
mkdir _build
mkdir _spec

pyinstaller ..\analyzer/analyzer.py ^
  --hidden-import winrt.windows.foundation.collections ^
  --paths ".." ^
  --clean ^
  --onefile ^
  --distpath _dist  ^
  --workpath _build ^
  --specpath _spec 


dir _dist

copy /b/y _dist\analyzer.exe ..\..\release\windows\analyzer.exe




