#ifndef XAOSDIALOG_H
#define XAOSDIALOG_H
#include <Window.h>
#include <View.h>
#include <Box.h>
#include "XaoSEvent.h"
#include "xmenu.h"
#include "ui.h"
#define MAXELEMENTS 20
class XaoSElement;
class XaoSDialog:public BWindow
{
public:
  typedef BWindow inherited;
    XaoSDialog (BRect w, CONST char *name, port_id port, uih_context * c);
    virtual ~ XaoSDialog (void);
  virtual void MessageReceived (BMessage * pMessage);
  static void cleanup (void);
private:
  const port_id mEventPort;
  struct uih_context *context;
  CONST char *name;

  BRect GetRect (BRect w, struct uih_context *c, CONST char *name);
  void DoApply ();
  void SendEvent (long eventCode, const XaoSEvent & event) const;

  XaoSElement *elements[MAXELEMENTS];
  CONST menuitem *item;
  CONST menudialog *dialog;
  int nitems;
  BBox *box;

  int width, textwidth, height;
  int sizes[MAXELEMENTS];
};
#endif
