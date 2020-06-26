How to create (pre)releases by using the scripts in this folder?
================================================================

In this folder we provide scripts that can create installation bundles
for various platforms.

We assume that you are already familiar with the
[steps for compiling XaoS](https://github.com/xaos-project/XaoS/wiki/Developer's-Guide#build-instructions).

Debian Linux and variants
-------------------------

Simply run the script `create-deb` from command line. See the comments in
the script in the first few rows to learn which packages must be
installed in advance.

Microsoft Windows
-----------------

Before running the script `deploy-win.bat` from command line you need to
add the path of tools `windeployqt.exe` and `binarycreator.exe` to the
environmental system variable PATH.

To avoid problems with finding certain files, you should make sure that
there is no special character in the full path of the `XaoS` folder.
Otherwise some files may be missing from the installation bundle (for
example, the .cat files).

MacOS
-----

Before running the script `deploy-mac` from command line you need to add
the path of tool `macdeployqt` to the environmental system variable PATH.
