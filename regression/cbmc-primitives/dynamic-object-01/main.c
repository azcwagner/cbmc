#include <assert.h>
#include <stdlib.h>

void main()
{
  char *p = malloc(1);
  free(p);

  assert(__CPROVER_DYNAMIC_OBJECT(p));
  assert(!__CPROVER_DYNAMIC_OBJECT(p));
}
