#include <Button.h>
#include <Locker.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Entry.h>
#include <Application.h>
#include <string.h>
#include <malloc.h>
#include <Message.h>
#include <StringView.h>
#include <TextControl.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <FilePanel.h>
#include "XaoSDialog.h"
#define SEPARATOR 5
#define BORDER 5
struct uih_context;
extern void be_help(struct uih_context *c, const char *name);

/* The dialog is created from XaoSElements holding the values and labels*/
class XaoSElement 
{
public:
	XaoSElement(const menudialog *dialog);
	XaoSElement(void);
	virtual ~XaoSElement(void);
	virtual void GetValue(dialogparam *param) = 0;
	virtual void GetMinimalSize(int *textwidth, int *width, int *height) = 0;
	virtual void SetIt(int xstart, int textwidth, int width, int top, int height, BView *view) = 0;
protected:
	const menudialog *dialog;
};
XaoSElement::XaoSElement(const menudialog *d)
:	dialog(d)
{
}
XaoSElement::~XaoSElement()
{
}

/* String elements implements input using BTextControl */
class XaoSAStringElement: public XaoSElement
{
public:
   typedef XaoSElement inherited;
 
   XaoSAStringElement(const menudialog *dialog);
   ~XaoSAStringElement();
   virtual void GetMinimalSize(int *textwidth, int *width, int *height);
   virtual void SetIt(int xstart, int textwidth, int width, int top, int height, BView *view);
   void SetDefault(const char *string);
protected:
   BTextControl *text;
private:
   char *string;
};
XaoSAStringElement::XaoSAStringElement(const menudialog *d)
:	inherited(d)
{
   BRect r(0,0,10,10);
   text = new BTextControl(r, d->question, d->question, "" , new BMessage('XaTc'));
   string=NULL;
}
void
XaoSAStringElement::SetDefault(const char *s)
{
   if (string) delete string;
   text->SetText(s);
   string=strdup(s);
}
XaoSAStringElement::~XaoSAStringElement()
{
  if(string) free(string);
}
void
XaoSAStringElement::GetMinimalSize(int *textwidth, int *width, int *height)
{
     text->ResizeToPreferred();
     *textwidth=(int)text->Divider()+10;
     BRect r=text->Bounds();
     *height=(int)r.bottom+1;
     *width=(int)r.right-*textwidth+11;
}
void
XaoSAStringElement::SetIt(int xstart, int textwidth, int width, int top, int height, BView *view)
{
     BRect r(xstart,top,xstart+width+textwidth,top+height);

     /* We need to re-create the widget, because it for some purpose refuses to get proper sizes for textview */

     delete text;
     text = new BTextControl(r, dialog->question, dialog->question, "" , new BMessage('XaTc'));
     if (string!=NULL) text->SetText(string);
     text->SetDivider(textwidth);
     view->AddChild(text);
}

/* Choice elements implements input using BMenuField */
class XaoSChoiceElement: public XaoSElement
{
public:
   typedef XaoSElement inherited;
 
   XaoSChoiceElement(const menudialog *dialog);
   ~XaoSChoiceElement();
   virtual void GetMinimalSize(int *textwidth, int *width, int *height);
   virtual void SetIt(int xstart, int textwidth, int width, int top, int height, BView *view);
   virtual void GetValue(dialogparam *param);
protected:
   BMenuField *field;
   BMenu *menu;
};
XaoSChoiceElement::XaoSChoiceElement(const menudialog *d)
:	inherited(d)
{
   const char * const *choices=(const char * const *)d->defstr;
   int i;
   BRect r(0,0,10,18);
   menu = new BPopUpMenu(d->question);
   menu->SetRadioMode(TRUE);
   for(i=0;choices[i];i++)
   {
     BMenuItem *item = new BMenuItem (choices[i], new BMessage('XaMn'));
     menu->AddItem (item);
     if (i==d->defint) item->SetMarked(TRUE);
   }
   field = new BMenuField(r, d->question, d->question, menu);
}
void
XaoSChoiceElement::GetValue(dialogparam *param)
{
   int i;
   for(i=0;!menu->ItemAt(i)->IsMarked();i++);
   param->dint=i;
}
XaoSChoiceElement::~XaoSChoiceElement()
{
}
void
XaoSChoiceElement::GetMinimalSize(int *textwidth, int *width, int *height)
{
     BFont font;

     field->GetFont(&font);
     *textwidth=(int)font.StringWidth(dialog->question)+10;
     *width=100;
     *height=(int)font.Size()+8;
}
void
XaoSChoiceElement::SetIt(int xstart, int textwidth, int width, int top, int height, BView *view)
{
     field->MoveTo(xstart,top);
     field->ResizeTo(width+textwidth, height);
     field->SetDivider(textwidth);
     view->AddChild(field);
}

/* String elements implements input using BTextControl */
class XaoSComplexElement: public XaoSAStringElement
{
public:
   typedef XaoSAStringElement inherited;
 
   XaoSComplexElement(const menudialog *dialog);
   ~XaoSComplexElement();
   virtual void GetMinimalSize(int *textwidth, int *width, int *height);
   virtual void SetIt(int xstart, int textwidth, int width, int top, int height, BView *view);
   virtual void GetValue(dialogparam *param);
protected:
   BTextControl *text2;
   BStringView *i;
};
XaoSComplexElement::XaoSComplexElement(const menudialog *d)
:	inherited(d)
{
   char s[256];
   sprintf(s,"%g",(double)dialog->deffloat);
   SetDefault(s);
   sprintf(s,"%g",(double)dialog->deffloat2);
   BRect r(0,0,10,10);
   text2 = new BTextControl(r, "+", "+", s , new BMessage('XaTc'));
   i = new BStringView(r, "i", "i");
}
XaoSComplexElement::~XaoSComplexElement()
{
}
void
XaoSComplexElement::GetMinimalSize(int *textwidth, int *width, int *height)
{
     inherited::GetMinimalSize(textwidth, width, height);
     *width*=2;
     text2->ResizeToPreferred();
}
void
XaoSComplexElement::SetIt(int xstart, int textwidth, int width, int top, int height, BView *view)
{
     char s[256];
     int label1=(int)text2->Divider();
     BFont font;

     i->GetFont(&font);
     int label2=(int)font.StringWidth("i")+4;
     BRect r(xstart+textwidth+(width-label1)/2,top,xstart+width+textwidth-label2,top+height);

     /* We need to re-create the widget, because it for some purpose refuses to get proper sizes for textview */

     delete text2;
     sprintf(s,"%g",(double)dialog->deffloat2);
     inherited::SetIt(xstart,textwidth,(width-label1-label2)/2, top, height, view);
     text2 = new BTextControl(r, "+", "+", s , new BMessage('XaTc'));
     text2->SetDivider(label1);
     view->AddChild(text2);
     i->MoveTo(xstart+width+textwidth-label2+2,top);
     i->ResizeTo(label2, height-4);
     view->AddChild(i);
}
void
XaoSComplexElement::GetValue(dialogparam *param)
{
   param->dcoord[0]=ui_getfloat((const char *)text->Text());
   param->dcoord[1]=ui_getfloat((const char *)text2->Text());
}

/* String elements implements input using BTextControl */
class XaoSFileElement: public XaoSAStringElement
{
public:
   typedef XaoSAStringElement inherited;
 
   XaoSFileElement(const menudialog *dialog, BWindow *target);
   ~XaoSFileElement();
   virtual void GetMinimalSize(int *textwidth, int *width, int *height);
   virtual void SetIt(int xstart, int textwidth, int width, int top, int height, BView *view);
   virtual void GetValue(dialogparam *param);
   void ShowFileSelector();
   int FileSelectorMessage(BMessage *m);
   static void cleanup();
protected:
   BButton *browse;
   static BFilePanel *openpanel, *savepanel;
   BFilePanel *panel;
   BMessenger *target;
   BWindow *looper;
   BLocker locker;
};
BFilePanel *XaoSFileElement::openpanel=NULL;
BFilePanel *XaoSFileElement::savepanel=NULL;
void
XaoSFileElement::ShowFileSelector()
{
   BMessage *message = new BMessage('XaFp');
   panel->Hide();
   message->AddPointer("Ptr",this);
   panel->SetMessage(message);
   target = new BMessenger(looper);
   panel->SetTarget(*target);
   if (dialog->type == DIALOG_OFILE) panel->SetSaveText(text->Text());
   panel->Show();
}
/* Convert entry structure back to string. I am not sure how much hazardeous
   this conversion is (especially for save) */
int
XaoSFileElement::FileSelectorMessage(BMessage *m)
{
   locker.Lock();
   if (dialog->type == DIALOG_IFILE) {
	BPath path;
   	entry_ref refs;
   	if (m->FindRef("refs", &refs) != B_OK) {locker.Unlock(); return 0;}
   	BEntry e(&refs,true);
   	e.GetPath(&path);
   	SetDefault(path.Path());
   } else {
        const char *name;
	BPath path;
   	entry_ref refs;
   	if (m->FindRef("directory", &refs)!=B_OK) return 0;
        if (m->FindString("name", &name) != B_OK) return 0;
	BDirectory dir(&refs);
	BEntry entry(&dir, name);
	entry.GetRef(&refs);
   	entry.GetPath(&path);
        {
		BFile file(&entry, B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
		if (file.InitCheck() != B_OK) {locker.Unlock(); return 0;}
		BNodeInfo ni(&file);
		if (!strcmp(dialog->defstr,"fract*.xpf"))
			ni.SetType("image/x-xaos-position");
		else if (!strcmp(dialog->defstr,"anim*.xaf"))
			ni.SetType("video/x-xaos-animation");
		else if (!strcmp(dialog->defstr,"fract*.png"))
			ni.SetType("image/png");
        }
	SetDefault(path.Path());
   }
   locker.Unlock();
   return 1;
}
XaoSFileElement::XaoSFileElement(const menudialog *d, BWindow *t)
:	inherited(d)
{
   BRect r(0,0,10,10);
   char string[256];
   int i,y;
   BMessage *message = new BMessage('XaFi');
   looper=t;
   message->AddPointer("Ptr",this);
   for(i=0,y=0;dialog->defstr[i];i++) if(dialog->defstr[i]!='*') string[y++]=dialog->defstr[i];
   string[y]=0;
   SetDefault(string);
   browse = new BButton(r, "Browse", "Browse", message);
   target=NULL;
   locker.Lock();
   if(openpanel==NULL) {
     openpanel=new BFilePanel(B_OPEN_PANEL, NULL, NULL, B_FILE_NODE, FALSE);
     savepanel=new BFilePanel(B_SAVE_PANEL, NULL, NULL, B_FILE_NODE, FALSE);
   }
   locker.Unlock();
   if (d->type == DIALOG_OFILE) panel=savepanel; else panel=openpanel;
}
XaoSFileElement::~XaoSFileElement()
{
   locker.Lock();
      if(openpanel || savepanel) {
      panel->Hide();
      panel->SetTarget(be_app_messenger); // Use Be_app_messenger as /dev/null
      }
      if (target) delete target;
}
void
XaoSFileElement::GetMinimalSize(int *textwidth, int *width, int *height)
{
     float w,h;
     inherited::GetMinimalSize(textwidth, width, height);
     browse->GetPreferredSize(&w, &h);
     *width+=(int)w+1;
     if(*height<h) *height=(int)h+1;
}
void
XaoSFileElement::SetIt(int xstart, int textwidth, int width, int top, int height, BView *view)
{
     float w,h;
     browse->GetPreferredSize(&w, &h);

     inherited::SetIt(xstart,textwidth,width-w, top, height, view);
     browse->MoveTo(xstart+width+textwidth-w,top);
     browse->ResizeTo(w, height);
     view->AddChild(browse);
}
void
XaoSFileElement::GetValue(dialogparam *param)
{
   param->dstring=strdup(text->Text());
}
void
XaoSFileElement::cleanup(void)
{
     if (openpanel) {delete openpanel; openpanel=NULL;}
     if (savepanel) {delete savepanel; savepanel=NULL;}
}

/* Simple derived classes from string element */
class XaoSFloatElement: public XaoSAStringElement
{
public:
   typedef XaoSAStringElement inherited;
   XaoSFloatElement(const menudialog *dialog);
   virtual void GetValue(dialogparam *param);
};


XaoSFloatElement::XaoSFloatElement(const menudialog *dialog)
:inherited(dialog)
{
   char s[256];
   sprintf(s,"%g",(double)dialog->deffloat);
   SetDefault(s);
}
void
XaoSFloatElement::GetValue(dialogparam *param)
{
   param->number=ui_getfloat((const char *)text->Text());
}
class XaoSIntElement: public XaoSAStringElement
{
public:
   typedef XaoSAStringElement inherited;
   XaoSIntElement(const menudialog *dialog);
   virtual void GetValue(dialogparam *param);
};


XaoSIntElement::XaoSIntElement(const menudialog *dialog)
:inherited(dialog)
{
   char s[256];
   sprintf(s,"%i",dialog->defint);
   SetDefault(s);
}
void
XaoSIntElement::GetValue(dialogparam *param)
{
   param->dint=dialog->defint;
   sscanf(text->Text(),"%i",&param->dint);
}
class XaoSStringElement: public XaoSAStringElement
{
public:
   typedef XaoSAStringElement inherited;
   XaoSStringElement(const menudialog *dialog);
   virtual void GetValue(dialogparam *param);
};


XaoSStringElement::XaoSStringElement(const menudialog *dialog)
:inherited(dialog)
{
   SetDefault(dialog->defstr);
}
void
XaoSStringElement::GetValue(dialogparam *param)
{
   param->dstring=strdup(text->Text());
}
class OkCancelDialog: public XaoSElement
{
public:
   typedef XaoSElement inherited;
 
   OkCancelDialog(const menudialog *dialog);
   ~OkCancelDialog();
   virtual void GetValue(dialogparam *param);
   virtual void GetMinimalSize(int *textwidth, int *width, int *height);
   virtual void SetIt(int xstart, int textwidth, int width, int top, int height, BView *view);
private:
   BButton *ok,*cancel,*apply, *help;
};
OkCancelDialog::OkCancelDialog(const menudialog *dialog)
: inherited(dialog)
{
   BRect r(0,0,10,10);
   ok=new BButton(r,"OkButton","Ok", new BMessage ('XaOk'));
   ok->MakeDefault(TRUE);
   cancel=new BButton(r,"Cancel","Cancel", new BMessage ('XaCn'));
   apply=new BButton(r,"Apply","Apply", new BMessage ('XaAp'));
   help=new BButton(r,"Help","Help", new BMessage ('XaHp'));
}
OkCancelDialog::~OkCancelDialog()
{
  //delete yes,cancel,apply;
}
void
OkCancelDialog::GetValue(dialogparam *param)
{
}
#define SPACE 2
#define BORDERWIDTH 3
void
OkCancelDialog::GetMinimalSize(int *textwidth, int *width, int *height)
{
     float w, h;
     int maxw;
     ok->GetPreferredSize(&w, &h);
     maxw=(int)w;
     cancel->GetPreferredSize(&w, &h);
     if (w>maxw) maxw=(int)w;
     apply->GetPreferredSize(&w, &h);
     if (w>maxw) maxw=(int)w;
     help->GetPreferredSize(&w, &h);
     if (w>maxw) maxw=(int)w;
     h+=2*BORDERWIDTH;  /* Default border makes it looking in different way */
     *textwidth=0;
     *width=maxw*4+SPACE*6+BORDERWIDTH*8;
     *height=(int)h+1;
}
void
OkCancelDialog::SetIt(int xstart, int textwidth, int width, int top, int height, BView *view)
{
     int wwidth=width+textwidth;
     int bwidth=wwidth/4-SPACE;
     view->AddChild(help);
     view->AddChild(cancel, help);
     view->AddChild(apply, cancel);
     view->AddChild(ok, apply);
     ok->MoveTo(xstart,top);
     ok->ResizeTo(bwidth, height);
     apply->MoveTo(xstart+wwidth/4+SPACE+BORDERWIDTH,top+BORDERWIDTH);
     apply->ResizeTo(bwidth - 2*BORDERWIDTH, height - 2*BORDERWIDTH);
     cancel->MoveTo(xstart+2*wwidth/4+SPACE+BORDERWIDTH,top+BORDERWIDTH);
     cancel->ResizeTo(bwidth - 2*BORDERWIDTH, height - 2*BORDERWIDTH);
     help->MoveTo(xstart+3*wwidth/4+SPACE+BORDERWIDTH,top+BORDERWIDTH);
     help->ResizeTo(bwidth - 2*BORDERWIDTH, height - 2*BORDERWIDTH);
}
BRect
XaoSDialog::GetRect(BRect root, struct uih_context *context, const char *name)
{
   int sizes[MAXELEMENTS];
   int textwidth=0;
   int width=0;
   int height=0;
   int i;
   item = menu_findcommand (name);
   dialog = menu_getdialog(context, item);
   BRect rect2(0,0,200,200);
   box = new BBox(rect2, "dialog", B_FOLLOW_ALL);
   for(nitems=0; dialog[nitems].question; nitems++);
   for(i=0;i<nitems;i++)
   {
	switch(dialog[i].type)
          { 
            case DIALOG_FLOAT:
		elements[i]=new XaoSFloatElement(dialog+i);
		break;
            case DIALOG_INT:
		elements[i]=new XaoSIntElement(dialog+i);
		break;
            case DIALOG_IFILE:
            case DIALOG_OFILE:
		elements[i]=new XaoSFileElement(dialog+i, this);
		break;
            case DIALOG_STRING:
            case DIALOG_KEYSTRING:
		elements[i]=new XaoSStringElement(dialog+i);
		break;
            case DIALOG_COORD:
		elements[i]=new XaoSComplexElement(dialog+i);
		break;
            case DIALOG_CHOICE:
		elements[i]=new XaoSChoiceElement(dialog+i);
          }
   }
   height=nitems*SEPARATOR;
   elements[i]=new OkCancelDialog(dialog+i);
   for(i=0;i<nitems+1;i++)
   {
      int tw,w,h;
      elements[i]->GetMinimalSize(&tw,&w,&h);
      if(tw>textwidth) textwidth=tw;
      if(w>width) width=w;
      height+=h;
      sizes[i]=h;
   }
   BRect rect(0,0,width+textwidth+2*BORDER,height+2*BORDER);
   //box = new BBox(rect, "dialog", B_FOLLOW_ALL);
   box->ResizeTo(width+textwidth+2*BORDER+1,height+2*BORDER+1);
   box->SetViewColor(216,216,216,0);
   int pos=BORDER;
   for(i=0;i<nitems+1;i++)
   {
     elements[i]->SetIt(5,textwidth,width,pos, sizes[i], box);
     pos+=sizes[i]+SEPARATOR;
   }
   if (nitems==1 && (dialog[0].type == DIALOG_OFILE || dialog[0].type == DIALOG_IFILE))  root.left=root.right=65536;
   BPoint location((root.left+root.right-rect.right)/2,(root.top+root.bottom-rect.bottom)/2);
   rect.OffsetBy(location);
   return rect;
}
XaoSDialog::XaoSDialog(BRect w, const char *name, port_id port, uih_context *c)
:inherited(GetRect(w,c,name),item->name, B_FLOATING_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
 mEventPort(port),
 context(c),
 name(name)
{
   Lock();
   AddChild(box);
   Show();
   if (nitems==1 && (dialog[0].type == DIALOG_OFILE || dialog[0].type == DIALOG_IFILE)) {
     Hide();
     ((XaoSFileElement *)elements[0])->ShowFileSelector();
   }
   Unlock();
}
XaoSDialog::~XaoSDialog(void)
//:~inherited()
{
   int i;
   for(i=0;i<nitems+1;i++)
    delete elements[i];
}
void
XaoSDialog::DoApply(void)
{
    dialogparam *p=(dialogparam *)calloc(sizeof(*p),nitems);
    int i;
    for (i=0;i<nitems;i++)
      elements[i]->GetValue(p+i);
    SendEvent(XaoSEvent::Dialog, XaoSEvent(item, p));
}
void
XaoSDialog::SendEvent(long eventCode, const XaoSEvent &event) const
{
	(void)write_port(mEventPort, eventCode, &event, sizeof(XaoSEvent));
}
void
XaoSDialog::cleanup(void)
{
   XaoSFileElement::cleanup();
}
void
XaoSDialog::MessageReceived(BMessage *message)
{
   XaoSFileElement *p;
   switch(message->what)
   {
       case 'XaFi': 
	   message->FindPointer("Ptr",(void **)&p);
	   p->ShowFileSelector();
       break;
       case 'XaFp': 
       case B_REFS_RECEIVED: 
	   message->FindPointer("Ptr",(void **)&p);
	   if(!p->FileSelectorMessage(message) && nitems==1) {PostMessage(B_QUIT_REQUESTED); break;}
	   if (nitems==1) DoApply();
       break;
       case B_CANCEL: 
	   if (nitems==1) PostMessage(B_QUIT_REQUESTED);
       break;
       case 'XaOk': DoApply();
       case 'XaCn': PostMessage(B_QUIT_REQUESTED); break;
       case 'XaAp': DoApply(); break;
       case 'XaHp': be_help(NULL,item->shortname); 
       break;
       default: inherited::MessageReceived(message);
   }
}

