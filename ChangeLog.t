  o  Fix aspect ratios in win32 driver
  o  Get exact screen sizes in win32 driver
  o  Fix crash in subwindow filter
  o  Itersmall driver works in 24bpp too.
  o  Documentation for gui_driver section.
  o  MAX macro removed or renamed to not conflict
     with MAX macro defined by some OSes (like BeOS or Windows)
  o  Removed some cut-and-paste-programming artefacts in antialiasing driver
  o  Fixed fixedcolor -> bitmap switch bug.
  o  Added separators to menus
  o  Fixed another fixedcolor switch bug
  o  BeOS changes:
     o  Added new version of Jens's driver
     o  Updated autoconf files to support BeOS
     o  Quit and changing of driver works now. So I can start thinking about
        the DirectWindow driver.
     o  Added support for BeOS to configure script
     o  Fixed Jens's BeOS port to work on Intel
     o  better resizing
     o  Added status line to Be GUI
     o  Support 15bpp, 16bpp and all the other weird modes.
     o  Added "about" to file menu
     o  Fullscreen mode support
     o  DirectWindow mode support
     o  DirectScreen mode support
