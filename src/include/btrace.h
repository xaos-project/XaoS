#ifndef BTRACE_H
#define BTRACE_H
#ifdef __cplusplus
extern "C"
{
#endif
  int boundarytrace (int x1, int y1, int x2, int y2, number_t * xpos,
		     number_t * ypos);
  int boundarytraceall (number_t * xpos, number_t * ypos);
#ifdef __cplusplus
}
#endif
#endif
