set (UI_HLP_SRC
     autopilot.c
     ui_helper.c
     menu.c
     play.c
     render.c
     playtext.c
     save.c
     messg.c
     wstack.c)

add_library (ui-hlp STATIC ${UI_HLP_SRC})
