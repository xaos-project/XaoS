/* Small library to handle catalog files
 */
#ifndef CATALOG_H
#define CATALOG_H
#define CHASHMAX 31 /*Just small hash table. Should be OK */
#include "xio.h"
struct varnames {
    struct varnames *left, *right;
    char *name;
    char *value;
};
typedef struct catalog {
    struct varnames *root[CHASHMAX];
} catalog_t;
/*Find text in catalog */
extern char *find_text(catalog_t *catalog, const char *name);
/*Load catalog from file */
extern catalog_t *load_catalog(xio_file f, const char **error);
/*Free memory used by catalog */
extern void free_catalog(catalog_t *);
#endif /*VARIABLE_H */
