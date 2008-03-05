#ifdef SFFE_USING
/*/////////////////////////////////////////////////////////////////////////////////////
// project : sFFe ( SegFault (or Segmentation Fault :) ) formula evalutaor )
// author  : Mateusz Malczak ( mateusz@malczak.info )
// wpage   : www.segfaultlabs.com/projects/sffe
///////////////////////////////////////////////////////////////////////////////////////
// special build for XaoS, for more info visit
// http://www.segfaultlabs.com/projects/sfXaos
/////////////////////////////////////////////////////////////////////////////////////*/

#include <stdlib.h>
#include <stdio.h>
 #ifdef linux
	#include <ctype.h>
 #endif
#include <string.h>

#include "sffe.h"
#ifdef SFFE_CMPLX_ASM
 #include "sffe_cmplx_asm.h"
#elif SFFE_CMPLX_GSL
 #include "sffe_cmplx_gsl.h"
#endif


#ifdef SFFE_DLL
	#define SFFE_EXPORT __declspec(dllexport)
#else
	#define SFFE_EXPORT
#endif

#define sfset(arg,val) \
	(arg)->value = (sfNumber*)malloc(sizeof(sfNumber));\
		if ( (arg)->value )  cmplxset( *((arg)->value), (val),0 ) ;

#define sfvar(p,parser,idx) (p)->value = (sfNumber*)((parser)->vars+idx)

/************************* custom function */
/* all used in this section variables are defined depanding on complex number realization */
sffunction* sffe_function( char *fn, size_t len )
{
 unsigned char idx;
  	for ( idx=5; idx<sffnctscount; idx+=1 ) /* sffnctscount - defined in sffe_cmplx_* file */
		 if ( !strncmp(fn,sfcmplxfunc[idx].name,len) )
			return (sffunction*)(sfcmplxfunc+idx);
 return NULL;
};

sffunction* sffe_operator( char op )
{
 switch( op )
 {
  case '^' : return sfcmplxfunc; break;
  case '+' : return sfcmplxfunc+1; break;
  case '-' : return sfcmplxfunc+2; break;
  case '*' : return sfcmplxfunc+3; break;
  case '/' : return sfcmplxfunc+4; break;
 };
 return NULL;
};

void *sffe_const( char* fn, size_t len, void *ptr )
{
 unsigned char idx = 3;
  	for ( idx=0; idx<sfvarscount; idx+=1 )
	 if ( !strncmp(fn,sfcnames[idx],len) ) 
	 {
	  sfcvals[idx]( (sfNumber*)ptr );
	  return ptr;
	 };
 return NULL;
};
/************************* custom function */


SFFE_EXPORT sffe *sffe_alloc(void)
{
 sffe *rp = (sffe*)malloc(sizeof(sffe));
  if ( !rp ) return NULL;
 memset(rp,0,sizeof(sffe));
 return rp;
};

SFFE_EXPORT void sffe_free(sffe **parser)
{
 sffe *p = *parser;
 unsigned int i=0,j;
	for (;i<p->argCount;i++)
	{
		for ( j=0;j<p->varCount;j++ )
			if ( p->args[i].value==p->varPtrs[j] ) 
			j=p->varCount;
		if ( j==p->varCount ) 
			if ( p->args[i].value ) 
				free( p->args[i].value );
	};
  if ( p->userf ) 
	free( p->userf );
  if ( p->expression )
        free( p->expression );
// free(p->statics);
 free(p->args);
 free(p->oprs);
 free(p->varChars);
 free(p->varPtrs);
 free(*parser);
 parser = NULL;
};

SFFE_EXPORT void sffe_eval2(sffe *const parser)
{
 register sfopr* optro;
 register sfopr* optr = parser->oprs;
 register sfopr* optrl = parser->oprs+parser->oprCount;
 optro = optr;
  for ( optr=optr; optr!=optrl; optr+=1, optro+=1 )
  {
		optro->arg->parg = optro->arg-1;
		optr->arg->parg = optr->f( optr->arg )->parg;
  };
};

SFFE_EXPORT sfNumber sffe_eval(sffe *const parser)
{
 register sfopr* optro;
 register sfopr* optr = parser->oprs;
 register sfopr* optrl = parser->oprs+parser->oprCount;
 optro = optr;
  for ( optr=optr; optr!=optrl; optr+=1, optro+=1 )
  {
		optro->arg->parg = optro->arg-1;
		optr->arg->parg = optr->f( optr->arg )->parg;
  };
 return *(parser->result);
};

SFFE_EXPORT void* sffe_regvar(sffe **parser, sfNumber *vptrs, char vchars)
{
 unsigned int i = (*parser)->varCount;
 (*parser)->varCount += 1;
 (*parser)->varPtrs = (sfNumber**)realloc( (*parser)->varPtrs, (*parser)->varCount*sizeof(sfNumber*) );
	if ( !(*parser)->varPtrs ) return NULL;
 (*parser)->varChars = (char*)realloc((*parser)->varChars, (*parser)->varCount );
	if ( !(*parser)->varChars ) return NULL;
 (*parser)->varPtrs[i] = vptrs;
 (*parser)->varChars[i] = toupper(vchars);
 return (void*)((*parser)->varPtrs+i);
};

SFFE_EXPORT void* sffe_regvars(sffe **parser, unsigned int cN, sfNumber **vptrs, char *vchars)
{
 unsigned int i = (*parser)->varCount;
 (*parser)->varCount += cN;
 (*parser)->varPtrs = (sfNumber**)realloc( (*parser)->varPtrs, (*parser)->varCount*sizeof(sfNumber*) );
	if ( !(*parser)->varPtrs ) return NULL;
 (*parser)->varChars = (char*)realloc((*parser)->varChars, (*parser)->varCount );
	if ( !(*parser)->varChars ) return NULL;
  for (cN=0; i<(*parser)->varCount; i+=1, cN+=1)
  {
   (*parser)->varPtrs[i] = vptrs[cN];
   (*parser)->varChars[i] = toupper(vchars[cN]);
  }; 
 return (void*)((*parser)->varPtrs+i);
};

SFFE_EXPORT sfNumber* sffe_varptr(sffe *const parser, char vchar )
{
 unsigned int i=0;
  while ( i<parser->varCount )
  {
	  if ( parser->varChars[i]==vchar ) return parser->varPtrs[i];
	i+=1;
  };
 return NULL;
};

SFFE_EXPORT sfNumber* sffe_setvar(sffe **parser, sfNumber *vptrs, char vchars)
{
 unsigned int i=0;
  while ( i<(*parser)->varCount )
  {
  	 if ( (*parser)->varChars[i]==vchars ) 
	 {
		sfNumber *ret = (*parser)->varPtrs[i];
		(*parser)->varPtrs[i] = vptrs;
		return ret;
	 };
	i+=1;
  };
 return NULL;
};

SFFE_EXPORT void* sffe_regfunc(sffe **parser, char *vname, unsigned int parcnt, sffptr funptr)
{ 
 sffunction *sff;
 unsigned short i;
 (*parser)->userf = (sffunction*)realloc( (*parser)->userf, ((*parser)->userfCount+1)*sizeof(sffunction) );
	if ( !(*parser)->userf ) return NULL;
 sff = (*parser)->userf+(*parser)->userfCount;
 /* 2.XI.2007 changed to get rid of warinings */
 strcpy(sff->name, vname);
 /* sff->name = (char*)malloc( strlen(vname) ); */
  for ( i=0; i<strlen(vname); i+=1 )
	sff->name[i] = (char)toupper((int)vname[i]);
 sff->parcnt = parcnt;
 sff->fptr = funptr;
 (*parser)->userfCount += 1;
 return (void*)sff;
};

unsigned char sffe_priority(char *chr) 
{
 switch ( *chr )
 { 
  case 'f': return 5; 
  case '^': return 3; 
  case '*': case '/': return 2;
  case '+': case '-':  return 1;
  default : return 0;
 };
};

void *sffe_variable(sffe *const p, char *fname, size_t len)
{
 unsigned int idx=0;
 if ( len==1 ) /*FIXME vars names with length > 1 should be allowed */
  for(; idx<p->varCount; idx+=1 )
	/* if ( !strncmp(fname,p->varChars[idx],len) ) */
	if ( p->varChars[idx]==*fname )
		return (void*)p->varPtrs[idx];
 return NULL;
};

sffunction* userfunction(const sffe* const p, char *fname, size_t len )
{
 unsigned char idx;
  	for ( idx=0; idx<p->userfCount; idx+=1 )
	 if ( !strncmp(fname,p->userf[idx].name,len) )
			return (sffunction*)(p->userf+idx);
 return NULL;
};

char sffe_donum(char **str)
{ /* parse number in format [-+]ddd[.dddd[e[+-]ddd]]  */
 unsigned char flag=0; /*bit 1 - dot, bit 2 - dec, bits 3,4 - stan, bits 5..8 - error */
  if ( **str=='-' ) { flag = 0x80; *str+=1; };
  if ( **str=='+' ) *str+=1;   
   while( !((flag>>4)&0x07) )
   {
	switch ( (flag&0x0f)>>2 ) 
	{
		case 0:/*0..9 */
			while( isdigit(**str) ) *str+=1; 
			 switch( **str )/*only '.' or 'E' allowed */
 			  {
			    case '.' : flag = (flag&0xf3)|4; break;
			    case 'E' : flag = (flag&0xf3)|8; break;
			   default : flag = 0x10;
			  };
			break;
		case 1:/*.  */
			  if ( flag&0x03 ) flag = 0x20; else *str += 1; /*no 2nd dot, no dot after E  */
			 flag = (flag&0xf2)|0x01;
			break;
		case 2:/*e  */
			 if ( flag&0x02 ) flag = 0x30; else *str += 1; /*no 2nd E */
				if ( !isdigit(**str) ) /*after E noly [+-] allowed */
				{
					if ( **str!='-' && **str!='+' ) flag = 0x40; 
						else *str+=1;
				};
			 flag = (flag&0xf1)|0x02;
			break;
	};
   };   
  if ( flag&0x80 ) flag^=0x80;
 return flag>>4;
};

char sffe_docmplx(char **str, sfarg **arg)
{ /* parse complex number in format { [-+]ddd[.dddd[e[+-]ddd]] ; [-+]ddd[.dddd[e[+-]ddd]] }  */
 char *chr, *chi;
 chr  = *str;
   if ( sffe_donum(str)>1 ) return 1;
   if ( *(*str)++!=';' ) return 2;
 chi = *str;
   if ( sffe_donum(str)>1 ) return 1;
   if ( *(*str)++!='}' ) return 2;

 cmplxset( *(*arg)->value, atof(chr), atof(chi) );
 return 0;
};

char sffe_doname(char **str)
{
   do{*str+=1;}while(isalnum(**str)||**str=='_');  
   if ( strchr("+-*/^~!@#$%&<>?\\:\"|",(int)**str) ) return 1; /*punctator  */
   if ( **str=='(' ) return 2; /* ( - funkcja  */
   if ( **str=='.' ) return 3; /*error :( bo to oznacza cos w stylu X. COS. PI.*/
  return 1;
};

SFFE_EXPORT int sffe_parse(sffe **parser, char *expression)
{
/**************var area */
 struct opstack__
 {
	#ifdef SFFE_DEVEL
	 char t;		//used only in debug mode to print stack notation on screen
	#endif
	 unsigned char parcnt;  //store number of parameters for functions 
	 sffptr f;
 };
  struct stack__ {	
    struct opstack__	*stck;
    unsigned int   	size;   //number of items on stack
    unsigned char  	tpro;   //priority of operator on top of stack
    struct stack__	*prev;
  } *stmp, *stack;
 sffunction **fnctbl;
 sffunction **f;
 sfarg  *arg, *argcnt;
 char *ech;
 char *ch1, *ch2;
 char *expcode; 		/*tokenized form : (f(n)+f(n))*f(n)-n (f-func, n-num,const) */
 unsigned int ui1, ui2;
 unsigned char opr;
 char err; 
 sffe *p;
/**************used defines */
#define MEMERROR	  1
#define UNBALANCEDBRACKES 2
#define INVALIDFUNCTION   3
#define INAVLIDNUMBER     4
#define UNKNOWNCONST	  5
#define OPERATOR	  6
#define STACKERROR	  7
#define PARCNTERROR	  8
#define NO_FUNCTIONS	  9
#define code(chr) \
			expcode = (char*)realloc(expcode,ui1+2);\
			expcode[ui1++] = chr;\
			ch2 = expcode+ui1-1;\
			opr = chr;\
			expcode[ui1] = '\0';
#define errset(errno) {\
			err = errno;\
			break;}
#define insertfnc(fnc) \
			for ( argcnt=p->args+p->argCount-1; argcnt>arg; argcnt-=1 )\
				argcnt->value = (argcnt-1)->value;\
			sfset(argcnt,-1.0);	
#ifdef SFFE_DEVEL
	#define sfpopstack(a)\
			{\
				stack->size-=1;\
				insertfnc(NULL);\
				printf("%c",stack->stck[stack->size].t);\
				p->oprs[ui1].arg = (sfarg*)arg;\
				p->oprs[ui1].f = stack->stck[stack->size].f;\
				ui1 += 1;\
				arg += 1;\
			};
#else 
	#define sfpopstack(a)\
			{\
				stack->size-=1;\
				insertfnc(NULL);\
				p->oprs[ui1].arg = (sfarg*)arg;\
				p->oprs[ui1].f = stack->stck[stack->size].f;\
				ui1 += 1;\
				arg += 1;\
			};
#endif
		
#ifdef SFFE_DEVEL
 printf("parse - BEGIN\n");
#endif
/**************** CODE */ 
 fnctbl = NULL;
 ech = expression;
 expcode = (char*)malloc(1);
 err = 0;
 //parser
 p = *parser;
	/* clear all internal structures */
		for (ui1=0;ui1<p->argCount;ui1+=1)
		{
			for ( ui2=0;ui2<p->varCount;ui2+=1 )
				if ( p->args[ui1].value==p->varPtrs[ui2] ) 
					ui2=p->varCount;
			if ( ui2==p->varCount ) //not found to be var
				if ( p->args[ui1].value ) 
					free( p->args[ui1].value );
		};
	free(p->args); p->args = NULL;	
	free(p->oprs); p->oprs = NULL;
		if ( p->expression ) free( p->expression );	
	p->expression = NULL;
 p->oprCount = 0;
 p->argCount = 0;
 p->expression = (char*)malloc(strlen( expression )+1);
 strcpy(p->expression,expression);
 ech = p->expression; 

#ifdef SFFE_DEVEL 
	printf("\n|-----------------------------------------\n+ > %s[%d] - parsing\n|-----------------------------------------\n",__FILE__,__LINE__);
 	printf("| input (dl.=%d) :|%s|\n",strlen(p->expression),p->expression);
#endif
 
/*! PHASE 1 !!!!!!!!! remove spaces, count brackets, change decimal separators ',' to '.', remove multiple operators eg. ++--++1 -> 1, -+++2 -> -2 */
 ch1 = NULL; 
 ui1 = 0; /*brackets */
 ch2 = ech;
   while(isspace(*ech))ech +=1; /* skip leading spaces*/
  while( *ech )
  {
   	 /*handle brackets and chaange ','->'.' */
	 switch ( *ech )
	 {
	  case '[' : *ech = '('; 
	   case '(' : ui1 += 1; 
	  break;
	  case ']' : *ech = ')';
	   case ')' : ui1 -= 1; 
	  break;
	  case ',' : *ech = '.';
	   break;
	 };
    *ch2 = (char)toupper((int)*ech);
    /*fix multiple arithm operators*/	 
	  if ( ch1 && strchr("+-/*^",(int)*ech) && strchr("+-/*^",(int)*ch1)  )
	  {	  
	    if ( *ch1=='-'&&*ech=='-' ) *ch1='+'; else
	     if ( *ch1=='-'&&*ech=='+' ) *ch1='-'; else
		  if ( *ch1=='+'&&*ech=='-' ) *ch1='-'; else
		   if( *ch1==*ech ) *ch1=*ech; else
		    if ( *ech=='-' ) ch1=++ch2; else
 		      if ( *ch1!=*ech ) { err = OPERATOR; break;	 };
	  } else {
		  ch1=ch2;
		  ch2+=1;	
 		 };
     do{ech +=1;}while(isspace(*ech));/*skip spaces*/
  };
  *ch2 = '\0';  
  p->expression = (char*)realloc( p->expression, strlen(p->expression)+1 );  
   if ( ui1 && !err ) err  = UNBALANCEDBRACKES;

#ifdef SFFE_DEVEL
  printf("| check (dl.=%d) :|%s|\n",strlen(p->expression),p->expression);
#endif
 
/*! PHASE 2 !!!!!!!! tokenize expression, lexical analizis (need optimizations) */
  *expcode = '\0';
  ch2 = NULL;
  ui1 = 0;
  ch1 = NULL; /*string starting position*/
  ech = p->expression;
  opr = '('; /* in case of leading '-' */
   while( *ech && !err )
   {
     ch1 = ech;

	if ( isalpha(*ech) ) 
	{			
	  switch ( sffe_doname(&ech) )
	  {
	  	case 1: /* const or variable */
			p->args = (sfarg*)realloc( p->args, (++p->argCount)*sizeof(sfarg) );
			  if (!p->args) errset(MEMERROR); 
			arg = p->args+p->argCount-1;
			arg->value = (sfNumber*)sffe_variable(p,ch1,(size_t)(ech-ch1)); 
			 if (!arg->value )
			 {
				sfset(arg, 10.0 );
				 if ( arg->value ) 
				 {
					if ( !sffe_const(ch1,(size_t)(ech-ch1),arg->value) ) 
						errset(UNKNOWNCONST);
				 } else errset(MEMERROR);
			 };
			opr = 'n';
			  break;
	  	case 2: /* function */
			fnctbl = (sffunction**)realloc(fnctbl,(p->oprCount+1)*sizeof(sffunction*));
			 if ( !fnctbl ) errset(MEMERROR); 
			f = fnctbl+(p->oprCount++);
			*f = NULL;
			 if ( p->userfCount )  
			   /*is it user defined function */
			   *f = (sffunction*)(void*)userfunction(p,ch1,(size_t)(ech-ch1)); 
				  if ( !*f )
					/*if not, is it build in function */
					*f = (sffunction*)(void*)sffe_function(ch1,(size_t)(ech-ch1)); 
				  /* if not -> ERROR */
 				  if ( !*f  ) errset(INVALIDFUNCTION);
			opr = 'f';
		  break;
		case 3 : /* what ? */
			errset(OPERATOR);
		  break;
	  };	
 	} else /* numbers (this part can be optimized) */
		/* is it a real number */
		if ( isdigit(*ech)||( strchr("/*^(",(int)opr)&&strchr("+-",*ech) )  ) 
		{ 
			ch1 = ech; /* st = 1;  */
			  if ( sffe_donum(&ech)>1 ) errset(INAVLIDNUMBER);
			/*epx*/
			p->args = (sfarg*)realloc( p->args, (++p->argCount)*sizeof(sfarg) );
			  if (!p->args) errset(MEMERROR); 
			arg = p->args+p->argCount-1;
			sfset( arg, atof(ch1) );
			/*epx*/
			opr = 'n';
		} else
			/* if not, it can be complex number */
#ifdef SFFE_COMPLEX
			if ( *ech=='{' )
			{
			 ech += 1;
			 p->args = (sfarg*)realloc( p->args, (++p->argCount)*sizeof(sfarg) );
			  if (!p->args) errset(MEMERROR); 
 			 arg = p->args+p->argCount-1;
			 sfset( arg, 0 );
			  if ( sffe_docmplx(&ech,&arg) ) errset(INAVLIDNUMBER);
			 opr = 'n';
			}
			else 
#endif
			 /* if not, we have operator */
			 { 
			    ch1 = (char*)sffe_operator(*ech);

		  	    	if ( ch1 )
			    	{
				 fnctbl = (sffunction**)realloc(fnctbl,(++p->oprCount)*sizeof(sffunction*));
						if ( !fnctbl ) errset(MEMERROR); 
					fnctbl[p->oprCount-1] = (sffunction*)ch1;
			    	};
			   ch1 = ech;
			   opr = *ech;
			   ech +=1 ;
			 };

	 
	  /* check if multiply sign skipped, nf, n(, )( */
 	  if ( !err && ui1>0 )		
	    if ( opr=='f' || opr=='n' || opr=='(' )
		  if ( *ch2=='n' || *ch2==')' )
		  {				
				ch1 = (char*)sffe_operator('*');
				fnctbl = (sffunction**)realloc(fnctbl,(++p->oprCount)*sizeof(sffunction*));
					if ( !fnctbl ) errset(MEMERROR); 
				  if ( opr=='f' )
				  {
					fnctbl[p->oprCount-1] = fnctbl[p->oprCount-2];
					fnctbl[p->oprCount-2] = (sffunction*)ch1;
				  } else fnctbl[p->oprCount-1] = (sffunction*)ch1;
				ch1 = (char*)(int)opr; /* trick! ]:-> */
				code('*');
				opr = (char)(int)ch1;
				ch1 = NULL;
		  };
	
	 code(opr);
	};
 
 ech = expcode;

#ifdef SFFE_DEVEL  
  printf("| compiled expr. :|%s|\n| operacje: %d\n| stale,zmienne: %d\n| stack not.: ",expcode,p->oprCount,p->argCount);
#endif

/*! PRE PHASE 3 !!!!! no operations in expression = single numeric value */
  if ( !p->oprCount && p->argCount==1 )
  {
	p->oprs = (sfopr*)malloc( p->argCount*sizeof(sfopr) );
	p->oprs[0].arg = (sfarg*)p->args;
	p->oprs[0].f = NULL;
	p->result = (sfNumber*)p->args->value;
  } else
/*! PHASE 3 !!!!! create sffe 'stack' notation ]:-> */
/* lots of memory operations are done here but no memory leaks should occur */
  if ( !err )  
   {
    ui1 = p->argCount + p->oprCount;
    p->args = (sfarg*)realloc( p->args, ui1*sizeof(sfarg) );	
    memset(p->args+p->argCount,0,p->oprCount*sizeof(sfarg));
    p->argCount = ui1;
    arg = p->args;
    p->oprs = (sfopr*)malloc( p->oprCount*sizeof(sfopr) );    
    ch1 = NULL; /* number */
	/* stacks ( stores operations and controls parameters count inside of brackts blocks ) */
	stack = (struct stack__*)malloc(sizeof(struct stack__));
	stack->size = 0; /* 0-stack is empty, but ready to write (one slot allocated), >0-number of element on stack */
	stack->stck = (struct opstack__*)malloc(sizeof(struct opstack__));
	stack->prev = NULL;
	stack->tpro = 0;
	memset(stack->stck,0,sizeof(struct opstack__));
    ui1 = 0; /* used in defines */
    f = fnctbl;
    ch2 = (char*)malloc(1); /* store number of parameters here */
    *ch2 = 0;
	
	while( *ech && !err )
	{			
		switch (*ech) 
		{
	/*  O */
		 case '+': case '-': case '*': case '/': case '^':
			if ( ch1 ) 
			{ 
				#ifdef SFFE_DEVEL 
					printf("%c",*ch1); 
				#endif
				arg+=1; 
			} 
				/* there is an operator on stack */
				if ( stack->size )
				{		 
				  ch1 = (char*)(int)sffe_priority(ech);
					/* with higher priority (so must go first), */
					 if ( (unsigned char)(int)ch1 >= (unsigned char)(stack->tpro&0x0f) )
					 {
						stack->tpro = (stack->tpro&0xF0)|((unsigned char)((int)ch1&0x0F));
						stack->stck = (struct opstack__*)realloc( stack->stck, (stack->size+1)*sizeof(struct opstack__));
					 }
					  else /* if not, lower priority operators must wait */
						 {
							 while( stack->size )
								sfpopstack(NULL);
							stack->size = 0;
							stack->stck = (struct opstack__*)realloc( stack->stck, sizeof(struct opstack__));
							stack->tpro = (stack->tpro&0xF0)|((unsigned char)((int)ch1&0x0F));	
						 };
				} else /* in case only one operator on stack ? */
				       /* i dont remember what does why this line is imporatant */
					stack->tpro = (stack->tpro&0xF0)|(sffe_priority(ech)&0x0F);

			#ifdef SFFE_DEVEL
				stack->stck[stack->size].t = *ech;
			#endif

		   stack->stck[stack->size].f = ((sffunction*)(*f))->fptr; /* get function pointer */
		   stack->stck[stack->size].parcnt = 0; /* for now skip params count */
		   stack->size += 1;
		   f+=1;
		   ch1 = NULL;
		 break;
	/* F  */
		 case 'f' :
			stack->stck = (struct opstack__*)realloc( stack->stck, (stack->size+1)*sizeof(struct opstack__));
			 #ifdef SFFE_DEVEL
				stack->stck[stack->size].t = *ech;
			 #endif

			stack->stck[stack->size].f = ((sffunction*)(*f))->fptr; /* get function pointer */

			ch2 = realloc( ch2, *ch2+2 );
			*ch2 += 1;
			ch2[(int)*ch2] = ((sffunction*)(*f))->parcnt;

			stack->size += 1;
			f += 1;
			ch1 = NULL;
		/* no break - handle bracket after function name *
	/* (  */
		 case '(' : 
			/* store current stack */
			stmp = (struct stack__*)malloc(sizeof(struct stack__));
			stmp->prev = stack;
			stack = stmp;
			stack->size = 0;
			stack->stck = (struct opstack__*)malloc(sizeof(struct opstack__));
				#ifdef SFFE_DEVEL
					stack->stck[0].t = '_';
				#endif			
			stack->tpro = 0x0;
			  if ( *ech=='f' ) 
			  {
				ech += 1;
				stack->tpro = 0x80;
			  };
			opr = 0;
		 break;
	/*  ; */
		 case ';':
			if ( ch1 ) 
			{ 
				#ifdef SFFE_DEVEL
					printf("%c",*ch1); 
				#endif
				arg+=1; 
				ch1 = NULL;
			};
			    	/* if there is something on stack, flush it we need to read next parameter */
				while( stack->size ) sfpopstack(NULL);			
			  
			  /* wrong number of parameters */
 			  if ( (int)ch2[(int)*ch2] == 1 ) errset(PARCNTERROR)

		      ch2[(int)*ch2] -= 1; 
		 break;
	/* )  */
		 case ')' : 
			  if ( ch1 ) 
			  { 
				#ifdef SFFE_DEVEL
					printf("%c",*ch1); 
				#endif
				arg+=1; 
			  } 
			ch1 = NULL;			
			opr = stack->tpro&0xF0;
				while( stack->size )
					sfpopstack(NULL);
			  if ( !stack->prev ) errset(STACKERROR);
			stmp = stack;
			free( stmp->stck );
			stack = stmp->prev;
			free(stmp);
			
				/* i was reading function */
				if ( opr ) 
				{	
				  /* wrong number of parameters */
  	 			  if ( (int)ch2[(int)*ch2]>1 ) errset(PARCNTERROR);
				    if (!err)
				    {
					ch2 = realloc( ch2, ch2[0] );
					*ch2 -= 1;
					sfpopstack(NULL);
						if ( stack->size )
							stack->stck = (struct opstack__*)realloc( stack->stck, (stack->size)*sizeof(struct opstack__));
				    };
				};
		 break;
	/* n */
		 case 'n' : ch1 = ech; break;
		};
	 ech += 1;
	};

    free(ch2); /* free */

	if ( !err )
	{
	  	if ( ch1 ) 
		{ 
			#ifdef SFFE_DEVEL
			printf("%c",*ch1); 
			#endif
		 	arg+=1; 
		} 

		while ( stack ) /*clean up stack*/
		{
				while( stack->size )
				{
					stack->size -= 1;
						#ifdef SFFE_DEVEL
							printf("%c",stack->stck[stack->size].t);
						#endif
					insertfnc(NULL);				
					p->oprs[ui1].arg = (sfarg*)arg;
					p->oprs[ui1].f = stack->stck[stack->size].f;
					ui1 += 1;
					arg += 1;			
				};
			free(stack->stck);
			stmp = stack->prev;
			free(stack);
			stack = stmp;
		}; 
		
	  /* set up formula call stack*/
	 (p->args)->parg = NULL;
		for ( ui1=1; ui1<p->argCount; ui1+=1)
			(p->args+ui1)->parg = (p->args+ui1-1);
	
#ifdef SFFE_DEVEL
			printf("\n| numbers :");
			for ( ui1=0; ui1<p->argCount; ui1+=1)
			{
				if ( (p->args+ui1)->value )		
					printf(" %g%+gI",real((*(p->args+ui1)->value)),imag((*(p->args+ui1)->value))); else
						printf(" [_]");
			};
	
			printf("\n| functions fnctbl:");
			for ( ui1=0;ui1<p->oprCount;ui1+=1)
				printf(" 0x%.6X [%s]",(int)fnctbl[ui1]->fptr,fnctbl[ui1]->name);
		
			printf("\n| functions used ptrs:");
			for ( ui1=0;ui1<p->oprCount;ui1+=1)
				printf(" 0x%.6X",(int)p->oprs[ui1].f);
#endif
	} else  /* prevent memory leaks */
		{
			while ( stack ) /* clean up stack */
			{
				free(stack->stck);
				stmp = stack->prev;
				free(stack);
				stack = stmp;
			}; 
		};
	/* set up evaluation result pointer (result is stored in last operation return) */
	p->result = (sfNumber*)(p->oprs+p->oprCount-1)->arg->value;
		if ( !p->result ) err = MEMERROR;
   };

 if ( err )
 {
#ifdef SFFE_DEVEL
 /* in debug mode report errors on stdout */
   printf("Parser error : ");
	switch ( err )
	{
		case MEMERROR : printf(" MEMORY ERROR!!"); break;
		case UNBALANCEDBRACKES : printf(" UNBALANCED BRACKETS!! : %s\n",ch1); break;
		case INVALIDFUNCTION : printf(" UNKNOWN FUNCTION!! : %s\n",ch1); break;
		case INAVLIDNUMBER : printf(" NUMBER FORMAT!! : %s\n",ch1); break;
		case UNKNOWNCONST : printf(" UNKOWN CONST or VAR NAME!! : %s\n",ch1); break;
		case OPERATOR :  printf(" UNKNOWN OPERATOR!! : %s\n",ch1); break;
		case STACKERROR : printf(" INTERNAL STACK CORRUPTED!! : %s\n",ch1); break;
		case PARCNTERROR : printf(" FUNCTION PARAMETERS ERROR!! : %s\n",ch1); break;
		case NO_FUNCTIONS : printf("Formula error ! ARE YOU KIDDING ME ?!? : %s",ch1); break;
	};
#endif
	/* try to store error message */
	if ( p->errormsg )
		switch ( err )
		{
			case MEMERROR : sprintf(p->errormsg, "Formula error ! MEMORY ERROR!!"); break;
			case UNBALANCEDBRACKES : sprintf(p->errormsg, "Formula error ! UNBALANCED BRACKETS!! : %s",ch1); break;
			case INVALIDFUNCTION : sprintf(p->errormsg, "Formula error ! UNKNOWN FUNCTION!! : %s",ch1); break;
			case INAVLIDNUMBER : sprintf(p->errormsg, "Formula error ! NUMBER FORMAT!! : %s",ch1); break;
			case UNKNOWNCONST : sprintf(p->errormsg, "Formula error ! UNKOWN CONST or VAR NAME!! : %s",ch1); break;
			case OPERATOR :  sprintf(p->errormsg, "Formula error ! UNKNOWN OPERATOR!! : %s",ch1); break;
			case STACKERROR : sprintf(p->errormsg, "Formula error ! INTERNAL STACK CORRUPTED!! : %s",ch1); break;
			case PARCNTERROR : sprintf(p->errormsg, "Formula error ! FUNCTION PARAMETERS ERROR!! : %s",ch1); break;
			case NO_FUNCTIONS : sprintf(p->errormsg, "Formula error ! ARE YOU KIDDING ME ?!? : %s",ch1); break;
		};
   	/* if error -> clean up */
		for (ui1=0;ui1<p->argCount;ui1+=1)
		{
			for ( ui2=0;ui2<p->varCount;ui2+=1 )
				if ( p->args[ui1].value==p->varPtrs[ui2] ) 
					ui2=p->varCount;
			if ( ui2==p->varCount )
				if ( p->args[ui1].value ) 
					free( p->args[ui1].value );
		};
	free(p->args); p->args = NULL;	
	free(p->oprs); p->oprs = NULL;
	p->oprCount = 0;
	p->argCount = 0;
		if ( p->expression ) free( p->expression );	
	p->expression = NULL;
 };

 /*undefine defines*/
 #undef sfpopstack
 #undef insertfnc
 #undef code
 #undef errset
 #undef MEMERROR
 #undef UNBALANCEDBRACKES
 #undef INVALIDFUNCTION
 #undef INAVLIDNUMBER
 #undef UNKNOWNCONST
 #undef OPERATOR
 #undef STACKERROR
 #undef PARCNTERROR
 free(expcode);
 free(fnctbl);

#ifdef SFFE_DEVEL
 printf("\nparse - END\n");
#endif
 return err;
};

#undef sfset
#undef sfvar

#endif
