/* Small library to handle catalog files
 */
#ifndef CATALOG_H
#define CATALOG_H
#define CHASHMAX 31		/*Just small hash table. Should be OK */
#include <xio.h>
#ifdef __cplusplus
extern "C"
{
#endif
  struct varnames
  {
    struct varnames *left, *right;
    char *name;
    char *value;
  };
  typedef struct catalog
  {
    struct varnames *root[CHASHMAX];
  }
  catalog_t;
/*Find text in catalog */
  extern char *find_text (catalog_t * catalog, CONST char *name);
/*Load catalog from file */
  extern catalog_t *load_catalog (xio_file f, CONST char **error);
/*Free memory used by catalog */
  extern void free_catalog (catalog_t *);
#ifdef __cplusplus
}
#endif
#endif				/*VARIABLE_H */
