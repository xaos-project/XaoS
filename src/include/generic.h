#undef p_set
#undef p_get
#undef p_setp
#undef bpp1
#undef p_getp
#undef p_add
#undef p_inc
#undef p_copy
#define p_set(ptr,val) (*(ptr)=(val))
#define p_get(ptr) (*(ptr))
#define p_setp(ptr,pos,val) ((ptr)[(pos)]=(val))
#define p_getp(ptr,pos) ((ptr)[(pos)])
#define p_add(ptr,val) ((ptr)+(val))
#define p_inc(ptr,val) ((ptr)+=(val))
#define p_copy(ptr1,val1,ptr2,val2) ((ptr1)[val1]=(ptr2)[val2])
