SET (UI_SRC
     drivers.c
     ui.c
     uihelp.c
     param.c
     fparams.c
     filesel.c
     uimenu.c
     pipecmd.c
     dialog.c)

add_library (ui STATIC ${UI_SRC})

add_subdirectory(ui-drv)
