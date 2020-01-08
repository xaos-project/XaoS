@echo off

where /q windeployqt
if errorlevel 1 (
    echo windeployqt is not in your path.
    exit /b
)

where /q binarycreator
if errorlevel 1 (
    echo binarycreator is not in your path.
    exit /b
)

cd /D "%~dp0\.."

windeployqt bin

rd /s /q installer\packages\net.sourceforge.xaos\data

mkdir installer\packages\net.sourceforge.xaos\data\bin
xcopy /s /e bin installer\packages\net.sourceforge.xaos\data\bin

mkdir installer\packages\net.sourceforge.xaos\data\catalogs
xcopy /s /e catalogs installer\packages\net.sourceforge.xaos\data\catalogs

mkdir installer\packages\net.sourceforge.xaos\data\tutorial
xcopy /s /e tutorial installer\packages\net.sourceforge.xaos\data\tutorial

mkdir installer\packages\net.sourceforge.xaos\data\examples
for /R examples %%f in (*.*) do copy %%f installer\packages\net.sourceforge.xaos\data\examples

binarycreator --offline-only -p installer\packages -c installer\config\config.xml xaossetup.exe
