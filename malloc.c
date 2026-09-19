/*
 * malloc.c: safe wrappers around malloc, realloc, free, strdup
 */

#ifndef NO_STDINT_H
#include <stdint.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "puzzles.h"

/*
 * smalloc should guarantee to return a useful pointer - we
 * can do nothing except die when it's out of memory anyway.
 */
void *smalloc(size_t size) {
    void *p;

    /*
     * malloc implementations are allowed to return NULL for
     * malloc(0). Apparently newlib in particular does so. If this
     * happens below, it will falsely trip the 'out of memory' check.
     * Pre-empt this by declaring that _our_ malloc and free wrappers
     * treat alloc(0) as a no-op, whether or not malloc itself does.
     */
    if (size == 0)
        return NULL;

#ifdef PTRDIFF_MAX
    if (size > PTRDIFF_MAX)
	fatal("allocation too large");
#endif
    p = malloc(size);
    if (!p)
	fatal("out of memory");
	check_abort();
    return p;
}

/*
 * sfree should guaranteeably deal gracefully with freeing NULL
 */
void sfree(void *p) {
    if (p) {
	free(p);
    }
}

/*
 * srealloc should guaranteeably be able to realloc NULL
 */
void *srealloc(void *p, size_t size) {
    void *q;

    /*
     * As above, handle zero-sized reallocations, and treat them as
     * equivalent to frees.
     */
    if (size == 0) {
        sfree(p);
        return NULL;
    }

#ifdef PTRDIFF_MAX
    if (size > PTRDIFF_MAX)
	fatal("allocation too large");
#endif
    if (p) {
	q = realloc(p, size);
    } else {
	q = malloc(size);
    }
    if (!q)
	fatal("out of memory");
	check_abort();
	return q;
}

/*
 * dupstr is like strdup, but with the never-return-NULL property
 * of smalloc (and also reliably defined in all environments :-)
 */
char *dupstr(const char *s) {
    char *r = smalloc(1+strlen(s));
    strcpy(r,s);
    return r;
}
