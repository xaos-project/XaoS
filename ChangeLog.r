 o Fix SMP thinkos
 o XaoS now works on plan9 again..
 o Fixed bug in docalc.c - julia code now works in truecolor modes
 o Fixed Win32 resizing bug
 o Workaround Win32 stat() bug (fix file selector problems)
 o ui_win32 driver redesigned and split into win32 driver and directX driver
 o DirectX DLLs are loaded at runtime, so they are not required for XaoS
 o resizing works
 o resolution selection dialog
 o Fixed SVGAlib driver lockup
 o handle "help file not found" correctly
 o New xerror library for signaling fatal error (should be redirected
   to messageboxes now)
 o Updated GGI driver, added support for changing visuals...
 o Add Win32 releated docs
 o Fix in filesel dialog
 o Fixed waitfunc timming
 o uih/c cleanups in ui_helper.c. ui_helper is now re-entrant
 o dialog for rendering functions to allow users of GUI centric systems do
   rendering.
