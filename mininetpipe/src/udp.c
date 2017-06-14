#include "harness.h"

void
Init (ArgStruct *p, int *pargc, char ***pargv)
{
    p->reset_conn = 0;
}

void
Setup (ArgStruct *p)
{
    p->s_str = (char *) malloc (PSIZE);
    p->r_str = (char *) malloc (PSIZE);
