#ifdef cpixel_t
#undef cpixel_t
#undef cpixeldata_t
#undef cppixel_t
#undef cpixeldata_t
#undef bpp
#undef bpp1
#undef p_add
#undef p_inc
#undef p_set
#undef p_get
#undef p_setp
#undef UNSUPPORTED
#undef p_getp
#undef p_copy
#endif
#ifndef STRUECOLOR24
#define UNSUPPORTED
#endif
#define cpixel_t pixel8_t
#define cppixel_t ppixel8_t
#define cpixeldata_t pixel32_t
#define bpp 3
/*FIXME this should make problems at small edian machines? */
#define p_set(ptr,val) ((*(pixel16_t *)(ptr))=(pixel16_t)(val),((ptr)[2])=(pixel8_t)((val)>>16))
#define p_get(ptr) (((pixel32_t)*(pixel16_t *)(ptr)+(pixel32_t)(*((ptr)+2)<<16)))
#define p_copy(ptr1,pos1,ptr2,pos2) (*((pixel16_t *)((ptr1)+(pos1)*3))=*(pixel16_t *)((ptr2)+(pos2)*3),(ptr1)[(pos1)*3+2]=(ptr2)[(pos2)*3+2])
#define p_setp(ptr,pos,val) p_set((ptr)+(pos)*3,val)
#define p_getp(ptr,pos) p_get((ptr)+(pos)*3)
#define p_add(ptr,pos) ((ptr)+(pos)*3)
#define p_inc(ptr,pos) ((ptr)+=(pos)*3)
