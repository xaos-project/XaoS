/* This routines are used to convert PossitionIO to xaos xio streams 
   They are horribly inefecient, because BeOS don't implement buffering.
   Hopefully IO is not the major problem in replaying XaoS files :)*/
#include <TranslationKit.h>
#include <malloc.h>
#include "xio.h"
struct bfile {
	BPositionIO *f;
	int eof;
};
static int sputc(int c, xio_file f)
{
	struct bfile *p=(struct bfile *)f->data;
	if (p->f->Write(&c, 1)!=1) {p->eof=1;return XIO_EOF;} else return 0;
	//return putc(c,(FILE *)f->data);
}
static int sputs(CONST char *c, xio_file f)
{
	int l=strlen(c);
	struct bfile *p=(struct bfile *)f->data;
	if (p->f->Write(&c, l)!=l) {p->eof=1;return XIO_EOF;} else return 0;
}
static int sungetc(int c, xio_file f)
{
	struct bfile *p=(struct bfile *)f->data;
	p->f->Seek(-1, SEEK_CUR);
	return 0;
}
static int sgetc(xio_file f)
{
	struct bfile *p=(struct bfile *)f->data;
	char c;
	if (p->f->Read(&c, 1)!=1) {p->eof=1;return XIO_EOF;} else return c;
}
static int sfeof(xio_file f)
{
	struct bfile *p=(struct bfile *)f->data;
	return p->eof;
}
static int sclose(xio_file f)
{
	free(f->data);
	free(f);
	return 0;
}
xio_file positionio_ropen(BPositionIO *io)
{
	xio_file f=(xio_file)calloc(1, sizeof(*f));
	struct bfile *d=(struct bfile *)calloc(1, sizeof(*d));
	f->data=d;
	d->eof=0;
	d->f=io;
	f->fclose=sclose;
	f->xeof=sfeof;
	f->fgetc=sgetc;
	f->fungetc=sungetc;
	xio_getc(f);
	xio_ungetc(0,f);
	return f;
}
xio_file possitionio_wopen(BPositionIO *io)
{
	xio_file f=(xio_file)calloc(1, sizeof(*f));
	struct bfile *d=(struct bfile *)calloc(1, sizeof(*d));
	f->data=d;
	d->eof=0;
	d->f=io;
	f->fputc=sputc;
	f->fputs=sputs;
	f->fclose=sclose;
	return f;
}
