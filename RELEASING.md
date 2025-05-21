# Steps to create a new release of XaoS

## Version bump

* Change XaoS_VERSION in src/include/config.h
* Change version number in doc/XaoS.lsm and installer/config/config.xml
* Change Version and ReleaseDate in installer/packages/net.sourceforge.xaos/meta/package.xml
* Add release information in NEWS and xdg/xaos.appdata.xml
* Check copyright year in tools/create-deb and src/ui/main.cpp and README.md
* Commit and push all changes

## Releasing

* Read the file README.md in the tools folder.
