 o Fixed problems with X driver
 o Antialiasing filter...
 o Fixed clipping in wstack.c
 o ! now activates command prompt
 o Builddialog now draws...
 o Antialiasing filter (slow, but should be nice for rendering
   animations/saving images)
 o Aded short name description into structure
 o Quite lot of dialog and menu handling moved from ui code to
   xmenu code
 o Added support for calling dialoged functions with just one paramter
   nicely
 o Some menu code made universal and moved into ui_helper.c
 o Save image and tutorial replay calls passfunc
 o menu library extended to handle hash table of short names
 o parsing of parameters is now controled by menu library
 o Lots of play code made generic and moved into ui_helper.c (it is now
   getting big grrr...)
 o menuroot is now handled by ui_helper library
 o Removed plenty of obsolette functions
 o Dialog code now handles complex number input nicely
 o Fixed numberous bugs
 o Checked tutorials and fixed forgoten stuff
