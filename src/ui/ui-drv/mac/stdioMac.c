/*
	stdioMac.c
	
	
	C-style file I/O for Apple Macintosh
	Allows Mac file refs to be used instead of FILE*,
	also lets file be stamped with type/creator code
	upon creation. This is necessary, for example, to
	get GIFs recognized by NetScape Navigator since they
	need to be marked as TEXT type.

	Useful for porting C-style file I/O without changing too
	many things. The value normally used as FILE* is actually 
	a short int. Simple #ifdef calls to fopen, etc. and replace
	with fopenMac, etc.
	
	Binary mode only. Full read/write access given.
	Many functions NOT supported (fseek, fputs, etc.).
	See stdioMac.h for list of supported functions.

	Revision History:
	
	When		Who		What
	------------------------------------------------------------------
	Nov 11/96	taps	Created
	Nov 14/96	taps	Added Mac-to-string filespec converter
	
*/


#include <stdio.h>
#include <string.h>

#include "stdioMac.h"

unsigned char *PathNameFromWD(long vRefNum, unsigned char *s);
unsigned char *PathNameFromDirID(long DirID, short vRefNum, unsigned char *s);
unsigned char *pStrcat(unsigned char *dest, unsigned char *src);
unsigned char *pStrcpy(unsigned char *dest, unsigned char *src);



FILE* fopenMac(const char *fname, const char *, OSType creator, OSType fileType)
{
	short	f;
	OSErr	err;
	char	fnamep[255];
	
	strcpy(fnamep, fname);
	c2pstr(fnamep);
	
	err = FSOpen((StringPtr)fnamep, 0, &f);
	if(err == fnfErr)
	{
		err = Create((StringPtr)fnamep, 0, creator, fileType);
		if(err == noErr)
		{
			err = FSOpen((StringPtr)fnamep, 0, &f);
			if(err != noErr)
				(void) FSDelete((StringPtr)fnamep, 0);
		}
		
	}
	if(err != noErr)
		f = nil;
			
	return (FILE*) f;
}


size_t fwriteMac(const void *buffer, size_t size, size_t count, FILE *fp)
{
	short f = (short) fp;
	
	OSErr err;
	long datacount = count * size;
	
	err = FSWrite(f, &datacount, buffer);
	if(err != noErr)
		datacount = 0;
		
	return((size_t) datacount);
}


int fputcMac(int c, FILE* fp)
{
	OSErr err;
	unsigned char buff;
	long			cd;
	
	short f = (short) fp;

	buff = (unsigned char) c;
	cd = 1;
	err = FSWrite(f, &cd, &buff);
	return (err == noErr ? c : EOF);
}

int fcloseMac(FILE *fp)
{
	return FSClose((short)fp) == noErr ? 0 : EOF;
}




unsigned char *FilespecFromSFReply(SFReply *sf, unsigned char *result)
{
	// Get the spelt-out version of a file (path and all) from an SFReply block.
	
	PathNameFromWD(sf->vRefNum, result);
	pStrcat(result, "\p:");
	pStrcat(result, sf->fName);
	return(result);
}


unsigned char *PathNameFromWD(long vRefNum, unsigned char *s)
{

    WDPBRec myBlock;

    myBlock.ioNamePtr = nil;
    myBlock.ioVRefNum = vRefNum;
    myBlock.ioWDIndex = 0;
    myBlock.ioWDProcID = 0;

    /* Change the Working Directory number in vRefnum into a real vRefnum */
    /* and DirID. The real vRefnum is returned in ioVRefnum, and the real */
    /* DirID is returned in ioWDDirID. */

    PBGetWDInfo(&myBlock,false);

    return(PathNameFromDirID(myBlock.ioWDDirID,myBlock.ioWDVRefNum,s));
}


unsigned char *PathNameFromDirID(long DirID, short vRefNum, unsigned char *s)
{
    CInfoPBRec  block;
    Str255      directoryName;
	OSErr		err;

    *s = 0;
    block.dirInfo.ioNamePtr = directoryName;
    block.dirInfo.ioDrParID = DirID;

    do {
        block.dirInfo.ioVRefNum = vRefNum;
        block.dirInfo.ioFDirIndex = -1;
        block.dirInfo.ioDrDirID = block.dirInfo.ioDrParID;

        err = PBGetCatInfo(&block,false);
        pStrcat(directoryName,"\p:");
        pStrcat(directoryName,s);
        pStrcpy(s,directoryName);
    } while (block.dirInfo.ioDrDirID != fsRtDirID);

		(*s) -= 1;	/* delete last character of string, which is a colon */
    return(s);
}

unsigned char *pStrcat(unsigned char *dest, unsigned char *src)
{
	long sLen;
	if(*src < 255 - *dest)
		sLen = *src;
	else
		sLen = 255 - *dest;
    BlockMoveData(src + 1, dest + *dest + 1, sLen);
    *dest += sLen;
    return (dest);
}

unsigned char *pStrcpy(unsigned char *dest, unsigned char *src)
{
    BlockMoveData(src, dest, (long) *src + 1); 
    return (dest);
}
