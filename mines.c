/*
 * mines.c: Minesweeper clone with sophisticated grid generation.
 * 
 * Still TODO:
 *
 *  - think about configurably supporting question marks.
 *
 * To include new grids:
 *
 *  - Add a line last in the GRIDLIST macro (if not last, then old
 *    game parameters and saved games will stop working).
 *
 *  - If the grid type has non-trivial grid description strings, then
 *    make a decision about how to generate one and add that to the
 *    switch statement in new_game_desc. Also update the switch
 *    statement in validate_desc, so it knows to expect a grid_desc.
 *
 *  - Check the MAX_EDGES and MAX_NEIGHBOURS defines and update if needed.
 *
 *  - If cursor should be supported, add a section in the
 *    update_neighbours function.
 *
 * -  The define GRID_DIAGNOSTICS might be helpful for the last three
 *    points.
 *
 *  - Find valid size parameters and add an entry in validate_params.
 *
 *  - Play it from the customs menu and experiment with parameters.
 *    When a good parameter set is found, consider it for inclusion in
 *    the menus.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#ifdef NO_TGMATH_H
#  include <math.h>
#else
#  include <tgmath.h>
#endif

#include "puzzles.h"
#include "grid.h"

#ifdef TIME_MINELAYOUT
#include <time.h>
#endif

enum {
    COL_BACKGROUND, COL_BACKGROUND2,
    COL_1, COL_2, COL_3, COL_4, COL_5, COL_6, COL_7, COL_8,
    COL_9, COL_10, COL_11, COL_12,
    COL_MINE, COL_BANG, COL_CROSS, COL_FLAG, COL_FLAGBASE, COL_QUERY,
    COL_HIGHLIGHT, COL_LOWLIGHT,
    COL_WRONGNUMBER,
    COL_CURSOR,
    COL_FLAGLIGHT,
    NCOLOURS
};

#define PREFERRED_TILE_SIZE 20
#define TILE_SIZE (ds->tilesize)
#ifdef SMALL_SCREEN
#define BORDER 8
#else
#define BORDER (TILE_SIZE * 3 / 2)
#endif
#define HIGHLIGHT_WIDTH (TILE_SIZE / 10 ? TILE_SIZE / 10 : 1)
#define OUTER_HIGHLIGHT_WIDTH (BORDER / 10 ? BORDER / 10 : 1)
#define GRID2SCREENX(x) (((x) - ds->xoff) * TILE_SIZE / ds->grid->tilesize + BORDER)
#define GRID2SCREENY(y) (((y) - ds->yoff) * TILE_SIZE / ds->grid->tilesize + BORDER)
#define SCREEN2GRIDX(x) (((x) - BORDER) * ds->grid->tilesize / TILE_SIZE + ds->xoff)
#define SCREEN2GRIDY(y) (((y) - BORDER) * ds->grid->tilesize / TILE_SIZE + ds->yoff)

/*
 * Direction to light source.  Length does not matter.  The direction
 * should not be parallel with any edge on any tile.  The direction
 * may have some impact on the appearance of a grid, e.g., the Kagome
 * grid looks less nice with xlight = -5 and ylight = -12.
 */
static const int xlight = -12, ylight = -5;


#define FLASH_FRAME 0.13F

/* Maximum number of edges on any face in the grids */
#define MAX_EDGES 14

/* Maximum number of neighbours of any face in the grids. */
#define MAX_NEIGHBOURS 16

#define GRIDLIST(A)                                             \
    A("Squares", SQUARE, SQUARE, 1)                             \
    A("Squares cyclic", SQUARE, SQUARE_CYCLIC, 1)               \
    A("Honeycomb", HONEYCOMB, HONEYCOMB, 1)                     \
    A("Honeycomb cyclic", HONEYCOMB, HONEYCOMB_CYCLIC, 1)       \
    A("Octagonal", OCTAGONAL2, OCTAGONAL2, 1)                   \
    A("Triangular", TRIANGULAR, TRIANGULAR, 1)                  \
    A("Triangular cyclic", TRIANGULAR, TRIANGULAR_CYCLIC, 1)    \
    A("Snub-Square", SNUBSQUARE, SNUBSQUARE, 1)                 \
    A("Cairo", CAIRO, CAIRO, 1)                                 \
    A("Kites", KITE, KITE, 0.9)                                 \
    A("Great Hexagonal", GREATHEXAGONAL, GREATHEXAGONAL, 1.2)   \
    A("Kagome", KAGOME, KAGOME, 1)                              \
    A("Floret", FLORET, FLORET, 0.8)                            \
    A("Dodecagonal", DODECAGONAL, DODECAGONAL, 1)               \
    A("Great-Dodecagonal", GREATDODECAGONAL, GREATDODECAGONAL, 1)     \
    A("Great-Great-Dodecagonal", GREATGREATDODECAGONAL, GREATGREATDODECAGONAL, 1) \
    A("Compass-Dodecagonal", COMPASSDODECAGONAL, COMPASSDODECAGONAL, 1)   \
    A("Penrose Rhombs", PENROSE_P3, PENROSE_P3, 0.8)            \
    A("Hats", HATS, HATS, 1.2)                                  \
    A("Penrose Kite/Dart", PENROSE_P2, PENROSE_P2, 0.8)         \
    A("Spectres", SPECTRES, SPECTRES, 1.2)                      \
    /* end of list */

#define GRID_NAME(title,gtype,mtype,scale) title,
#define GRID_CONFIG(title,gtype,mtype,scale) ":" title
#define GRID_MINESTYPE(title,gtype,mtype,scale) MINES_GRID_ ## mtype,
#define GRID_GRIDTYPE(title,gtype,mtype,scale) GRID_ ## gtype,
#define GRID_SCALE(title,gtype,mtype,scale) scale,

enum game_type { GRIDLIST(GRID_MINESTYPE) };
static char const *const gridnames[] = { GRIDLIST(GRID_NAME) };
#define GRID_CONFIGS GRIDLIST(GRID_CONFIG)
static grid_type grid_types[] = { GRIDLIST(GRID_GRIDTYPE) };
#define NUM_GRID_TYPES (sizeof(grid_types) / sizeof(grid_types[0]))
static float grid_scale[] = { GRIDLIST(GRID_SCALE) };

enum {
  PREF_HIGHLIGHT_FLAGS,
  N_PREF_ITEMS
};

struct game_params {
    int w, h, n;
    enum game_type type;
    bool unique;

    /* For non-interactive generation, you can set this to override
     * the randomised first-click location. */
    int first_click_tile;
};

struct mine_layout {
    /*
     * This structure is shared between all the game_states for a
     * given instance of the puzzle, so we reference-count it.
     */
    int refcount;
    bool *mines;
    /*
     * If we haven't yet actually generated the mine layout, here's
     * all the data we will need to do so.
     */
    int n;             /* nominal mine count, provided they all fit */
    bool unique;
    random_state *rs;
    midend *me;		       /* to give back the new game desc */
    /*
     * After we generate a layout on the first click, we want to
     * remember what the location of that click was, so that we can
     * mark the square if the user undoes the click, so they know
     * which is the safe starting square.
     */
    int start_tile;
};

struct game_state {
    int w, h, n;
    bool dead, won, used_solve;
    struct mine_layout *layout;	       /* real mine positions */
    struct grid_info *grid;
    signed char *board;			       /* player knowledge */
    /*
     * Each item in the `board' array is one of the following values:
     * 
     * 	- 0 to MAX_NEIGHBOURS mean the square is open and has a
     * 	  surrounding mine count.
     * 
     *  - -1 means the square is marked as a mine.
     * 
     *  - -2 means the square is unknown.
     * 
     * 	- -3 means the square is marked with a question mark
     * 	  (FIXME: do we even want to bother with this?).
     * 
     * 	- 64 means the square has had a mine revealed when the game
     * 	  was lost.
     * 
     * 	- 65 means the square had a mine revealed and this was the
     * 	  one the player hits.
     * 
     * 	- 66 means the square has a crossed-out mine because the
     * 	  player had incorrectly marked it.
     */
};

struct tile_info {
    /* bounding box in screen coordinates plus one pixel */
    int bb_xmin, bb_ymin, bb_xmax, bb_ymax;
    int order;
    int *coords;
};

struct rim_info {
    int bb_xmin, bb_ymin, bb_xmax, bb_ymax;
    int coords[2*4];  /* This is double storage: half of the
                       * coordinates are also in the next neighbour.
                       * However, trying to put everything into one
                       * array to avoid this double storage
                       * complicates the code significantly.
                       */
};

struct grid_info {
    /*
     * This structure is shared between all the game_states for a
     * given instance of the puzzle, so we reference-count it.
     */
    int refcount;
    enum game_type type;
    grid *game_grid;
    struct tile_info *tiles;
    struct rim_info *rims;
    grid_dot **rim_dots;
    char *desc;
    int tilesize; /* tile size from generated grid, scaled for Mines */
    int ntiles;   /* Number of tiles/faces in grid */
    int nrims;    /* Number of rim edges */
    int nsize;    /* size of neighbours array per tile */
    int *neighbours;
    /*
     * neighbours is a 2D-array flattened, thus indexed by i*nsize + j
     * where i is the index of the tile we want the neighbours of and
     * j is 0 ... nsize-1.  The value is the index of the tile.  The
     * array is filled with -1.
     */

    bool has_cursor;  /* true if the cursor movements arrays are defined. */
    int *cursor_right, *cursor_left, *cursor_up, *cursor_down;
};

/* ----------------------------------------------------------------------
 * Grid generation and handling routines.
 */

/*
 * Insert a neighbour in the neighbours array but check first that it
 * is not there already.  Assumes that nind is already filled with -1.
 */
static void insert_neighbour(int *nind, int neighbour)
{
    int i = 0;
    while (nind[i] >= 0) {
        if (nind[i] == neighbour)
            return;
        i++;
    }
    nind[i] = neighbour;
}

static void update_neighbours(const game_params *params, struct grid_info *gi)
{
    int x, y, dx, dy;
    int i, j, k, ni, base;
    int w = params->w;
    int h = params->h;
    grid_face *new_face;
    int max_neighbours = 0;
    int num_neighbours;
    int *nind;

    switch (params->type) {
    case MINES_GRID_SQUARE_CYCLIC:
        gi->nsize = 8;
        nind = snewn(w*h*gi->nsize, int);
        for ( i = 0; i < w*h*gi->nsize; i++)
            nind[i] = -1;

        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                i = (y*w + x) * gi->nsize;
                for (dy = -1; dy <= +1; dy++) {
                    for (dx = -1; dx <= +1; dx++) {
                        if (dx != 0 || dy != 0) {
                            ni = ((y+dy+h)%h)*w+(x+dx+w)%w;
                            insert_neighbour(&nind[i], ni);
                        }
                    }
                }
            }
        }
        break;
    case MINES_GRID_HONEYCOMB_CYCLIC:
        gi->nsize = 6;
        nind = snewn(w*h*gi->nsize, int);
        for ( i = 0; i < w*h*gi->nsize; i++)
            nind[i] = -1;
        /* We relay on the width to be even here! */

        /* First even tiles */
        for (i = 0; i < w*h; i+=2) {
            bool top = i < w;
            bool bot = i >= w*(h-1);
            bool left = i%w == 0;
            int me = i*gi->nsize;
            /* up left */
            if (!top&&!left)
                insert_neighbour(&nind[me], i - (w+1));
            else if (top && left)
                insert_neighbour(&nind[me], w*h - 1);
            else if (left)
                insert_neighbour(&nind[me+0], i - 1);
            else
                insert_neighbour(&nind[me], i + (w*(h-1) - 1));
            /* up */
            insert_neighbour(&nind[me], (!top)?i-w: i+w*(h-1));
            /* up right */
            insert_neighbour(&nind[me], (!top)?i-(w-1): i+(w*(h-1)+1));
            /* down right */
            insert_neighbour(&nind[me], i+1);
            /* down */
            insert_neighbour(&nind[me], (!bot)?i+w: i-w*(h-1));
            /* down left */
            insert_neighbour(&nind[me], (!left)?i-1:i+(w-1));
        }

        /* Then odd tiles */
        for (i = 1; i < w*h; i+=2) {
            bool top = i < w;
            bool bot = i >= w*(h-1);
            bool right = i%w == w-1;
            int me = i*gi->nsize;
            /* up left */
            insert_neighbour(&nind[me], i-1);
            /* up */
            insert_neighbour(&nind[me], (!top)?i-w: i+w*(h-1));
            /* up right */
            insert_neighbour(&nind[me], (!right)?i+1:i-(w-1));
            /* down right */
            if (!bot&&!right)
                insert_neighbour(&nind[me],  i+(w+1));
            else if (bot && right)
                insert_neighbour(&nind[me], 0);
            else if (right)
                insert_neighbour(&nind[me], i+1);
            else
                insert_neighbour(&nind[me], i - (w*(h-1)-1));
            /* down */
            insert_neighbour(&nind[me], (!bot)?i+w: i-w*(h-1));
            /* down left */
            insert_neighbour(&nind[me], (!bot)?i+(w-1): i-(w*(h-1)+1));
        }
        break;
    case MINES_GRID_TRIANGULAR_CYCLIC:
        /* Uses old style triangular */
        gi->nsize = 12;
        nind = snewn(2*w*h*gi->nsize, int);
        for ( i = 0; i < 2*w*h*gi->nsize; i++)
            nind[i] = -1;
        for (j = 0; j < h; j++) {
            int jm = (j-1+h)%h;
            int jp = (j+1)%h;
            for (i = 0; i < 2*w; i++) {
                int im = (i-1+2*w)%(2*w);
                int imm = (i-2+2*w)%(2*w);
                int ip = (i+1)%(2*w);
                int ipp = (i+2)%(2*w);
                int me = (j*2*w+i)*gi->nsize;
                if ((i+j)%2 == 0) {
                    /* top down */
                    insert_neighbour(&nind[me], jm * 2*w + imm);
                    insert_neighbour(&nind[me], jm * 2*w + im);
                    insert_neighbour(&nind[me], jm * 2*w + i);
                    insert_neighbour(&nind[me], jm * 2*w + ip);
                    insert_neighbour(&nind[me], jm * 2*w + ipp);
                    insert_neighbour(&nind[me], j  * 2*w + imm);
                    insert_neighbour(&nind[me], j  * 2*w + im);
                    insert_neighbour(&nind[me], j  * 2*w + ip);
                    insert_neighbour(&nind[me], j  * 2*w + ipp);
                    insert_neighbour(&nind[me], jp * 2*w + im);
                    insert_neighbour(&nind[me], jp * 2*w + i);
                    insert_neighbour(&nind[me], jp * 2*w + ip);
                } else {
                    /* top up */
                    insert_neighbour(&nind[me], jm * 2*w + im);
                    insert_neighbour(&nind[me], jm * 2*w + i);
                    insert_neighbour(&nind[me], jm * 2*w + ip);
                    insert_neighbour(&nind[me], j  * 2*w + imm);
                    insert_neighbour(&nind[me], j  * 2*w + im);
                    insert_neighbour(&nind[me], j  * 2*w + ip);
                    insert_neighbour(&nind[me], j  * 2*w + ipp);
                    insert_neighbour(&nind[me], jp * 2*w + imm);
                    insert_neighbour(&nind[me], jp * 2*w + im);
                    insert_neighbour(&nind[me], jp * 2*w + i);
                    insert_neighbour(&nind[me], jp * 2*w + ip);
                    insert_neighbour(&nind[me], jp * 2*w + ipp);
                }
            }
        }

        break;
    default:

        /* Phase 1: Calculate maximum number of neighbours. */
        for (i = 0; i < gi->ntiles; i++) {
            num_neighbours = 0;
            for (j = 0; j < gi->game_grid->faces[i]->order; j++) {
                for (k = 0; k < gi->game_grid->faces[i]->dots[j]->order; k++) {
                    if (gi->game_grid->faces[i]->dots[j]->faces[k] != NULL)
                        num_neighbours += 1;
                }
                /* One of those faces we counted was the face we counting
                 * neighbours of.
                 */
                num_neighbours -= 1;
                if (gi->game_grid->faces[i]->edges[j]->face1 != NULL &&
                    gi->game_grid->faces[i]->edges[j]->face2 != NULL)
                    /* An "edge neighbour" have been counted twice since
                     * it belongs to both the edges end dots.
                     */
                    num_neighbours -= 1;
            }
            max_neighbours = max(max_neighbours, num_neighbours);
        }
#ifdef GRID_DIAGNOSTICS
        printf("Max number of neighbours: %d\n", max_neighbours);
#endif
        assert(max_neighbours <= MAX_NEIGHBOURS);
        gi->nsize = max_neighbours;

        /* Phase 2: Find the neighbours and fill in the neighbours array. */
        nind = snewn(gi->ntiles * max_neighbours, int);
        for (i = 0; i < gi->ntiles * max_neighbours; i++)
            nind[i] = -1;

        for (i = 0; i < gi->ntiles; i++) {
            base = i * max_neighbours;
            for (j = 0; j < gi->game_grid->faces[i]->order; j++) {
                for (k = 0; k < gi->game_grid->faces[i]->dots[j]->order; k++) {
                    new_face = gi->game_grid->faces[i]->dots[j]->faces[k];
                    if (new_face == NULL || new_face->index == i)
                        continue;
                    insert_neighbour(&nind[base], new_face->index);
                }
            }
        }
    }

    gi->neighbours = nind;

#ifdef GRID_DIAGNOSTICS
    {
        /* Do not halt on error here.  It is rather helpful to be able
         * to play around with the cursor in the grid and learn about
         * index numbers of tiles.
         */

        bool found;
        for (i = 0; i < gi->ntiles; i++)
            for (j = 0; j < gi->nsize; j++)
                if ( nind[i*gi->nsize + j] < -2 ||
                     nind[i*gi->nsize + j] >= gi->ntiles) {
                    printf("Neighbour array error nind[%d*nsize+%d] = %d\n",
                           i, j, nind[i*gi->nsize + j]);
                }

        for (i = 0; i < gi->ntiles; i++) {
            for (j = 0; j < gi->nsize; j++) {
                int tile = nind[i*gi->nsize+j];
                if (tile >= 0) {
                    found = false;
                    for (k = 0; k < gi->nsize; k++) {
                        if (nind[tile*gi->nsize+k] == i) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        printf("Neighbour array error: tile %d is "
                               "neghobour to %d but not the reverse\n",
                               i, tile);
                    }
                }
            }
        }
    }
#endif
}

static void update_cursor(const game_params *params, struct grid_info *gi)
{
    int i, j;
    int w = params->w;
    int h = params->h;
    int *cur_right, *cur_left, *cur_up, *cur_down;


    /* Update cursor movement arrays */
    cur_right = snewn(gi->ntiles, int);
    cur_left = snewn(gi->ntiles, int);
    cur_up = snewn(gi->ntiles, int);
    cur_down = snewn(gi->ntiles, int);

    gi->has_cursor = true;
    switch (params->type) {
    case MINES_GRID_SQUARE_CYCLIC:
        for (i = 0; i < w*h; i++) {
            cur_right[i] = (i%w != w-1) ? i+1 : i-(w-1);
            cur_left[i] = (i%w != 0) ? i-1 : i+(w-1);
            cur_up[i] = (i >= w) ? i - w : i + w*(h-1);
            cur_down[i] = (i < w * h - w) ? i + w : i-w*(h-1);
        }
        break;
    case MINES_GRID_HONEYCOMB:
        for (i = 0; i < w*h; i ++) {
            bool top = i < w;
            bool bot = i >= w*(h-1);
            bool left = i%w == 0;
            bool right = i%w == w-1;
            if ((i%w)%2 == 0) {
                cur_right[i] = (!top && !right) ? i-(w-1) : -1;
                cur_left[i] = (!left) ? i-1 : -1;
                cur_up[i] = (!top) ? i - w : -1;
                cur_down[i] = (!bot) ? i + w : -1;
            } else {
                cur_right[i] = (!right) ? i+1 : -1;
                cur_left[i] = (!bot) ? i+(w-1) : -1;
                cur_up[i] = (!top) ? i - w : -1;
                cur_down[i] = (!bot) ? i + w : -1;
            }
        }
        break;
    case MINES_GRID_HONEYCOMB_CYCLIC:

        /* First even tiles */
        for (i = 0; i < w*h; i += 1) {
            bool top = i < w;
            bool bot = i >= w*(h-1);
            bool left = i%w == 0;
            bool right = i%w == w-1;
            if (i%2 == 0) {
                cur_right[i] = (!top) ? i-(w-1) : i+(w*(h-1)+1);
                cur_left[i] = (!left) ? i-1 : i+(w-1);
                cur_up[i] = (!top) ? i - w : i+w*(h-1);
                cur_down[i] = (!bot) ? i + w : i-w*(h-1);
            } else {
                cur_right[i] = (!right) ? i+1 : i-(w-1);
                cur_left[i] = (!bot) ? i+(w-1) : i-(w*(h-1)+1);
                cur_up[i] = (!top) ? i - w : i+w*(h-1);
                cur_down[i] = (!bot) ? i + w : i-w*(h-1);
            }
        }
        break;
    case MINES_GRID_TRIANGULAR_CYCLIC:
        /* Uses old style triangular */
        for (j = 0; j < h; j++) {
            bool top = j == 0;
            bool bot = j == h-1;
            for (i = 0; i < 2*w; i++) {
                bool left = i == 0;
                bool right = i == 2*w-1;
                int k = j*2*w+i;
                cur_right[k] = (!right)? k + 1 : k - (2*w - 1);
                cur_left[k] = (!left)? k - 1 : k + (2*w - 1);
                cur_up[k] = (!top)? k - 2*w : k + 2*w*(h-1);
                cur_down[k] = (!bot)? k + 2*w : k - 2*w*(h-1);
            }
        }
        break;
    case MINES_GRID_TRIANGULAR:
        for (j = 0; j < 2*(h/2); j++) {
            bool top = j == 0;
            bool bot = j == h-1;
            for (i=0; i < 2*w+1; i++) {
                int k = j*(2*w+1) + i;
                bool left = i == 0;
                bool right = i == w;
                bool even = i <= w;
                if (!bot)
                    cur_down[k] =  k + 2*w+1;
                else
                    cur_down[k] = -1;
                if (!top)
                    cur_up[k] =  k - (2*w+1);
                else
                    cur_up[k] =  -1;
                if (!even)
                    cur_left[k] = k - (w+1);
                else if (!left)
                    cur_left[k] = k + w;
                else
                    cur_left[k] = -1;
                if (!even)
                    cur_right[k] = k - w;
                else if(!right)
                    cur_right[k] = k + (w+1);
                else
                    cur_right[k] = -1;
            }
        }
        if (h%2 == 1) {

            /* First fix the downs on the second last row */
            if (h > 1) {
                j = h-2;
                for (i=0; i < 2*w+1; i++) {
                    int k = j*(2*w+1) + i;
                    bool left = i == 0;
                    bool right = i == w;
                    bool even = i <= w;
                    if (left || right)
                        cur_down[k] = -1;
                    else if (even)
                        cur_down[k] = k + 2*w;
                    else
                        cur_down[k] = k + 2*w-1;
                }
            }
            /* Last row: first and last triangle is missing and the
             * rest renumbered accordingly.
             */
            j = h-1;
            for (i=0; i < 2*w-1; i++) {
                int k = j*(2*w+1) + i;
                bool left = i == w-1;
                bool right = i == 2*w-2;
                bool even = i <= w-2; /* counting the missing triangles */
                bool top = j == 0;
                cur_down[k] = -1;
                if (top)
                    cur_up[k] = -1;
                else if (even)
                    cur_up[k] = k - 2*w;
                else
                    cur_up[k] = k - (2*w-1);
                if (even)
                    cur_left[k] = k + (w-1);
                else if (!left)
                    cur_left[k] = k - w;
                else
                    cur_left[k] = -1;
                if (even)
                    cur_right[k] = k + w;
                else if(!right)
                    cur_right[k] = k - (w-1);
                else
                    cur_right[k] = -1;
            }
        }
        break;
    case MINES_GRID_SQUARE:
    case MINES_GRID_SNUBSQUARE:
    case MINES_GRID_OCTAGONAL2:
        /*
         * Automatic generation of neighbours.  This will work for
         * grids where the movement goes to edge neighbours
         * (disculaifies Triangles and cyclic grids), the direction
         * of the edges makes it clear which one is up etc, and the
         * order is either four or less, or the extra edges are
         * exactly 45 degrees in grid coordinates (enables
         * Octagonal2).
         */

        for (i=0; i < gi->ntiles; i++) {
            cur_right[i] = -1;
            cur_left[i] = -1;
            cur_up[i] = -1;
            cur_down[i] = -1;
        }
        for (i = 0; i < gi->ntiles; i++) {
            int order = gi->game_grid->faces[i]->order;
            for (j = 0; j < order; j++) {
                grid_face *other;
                if (gi->game_grid->faces[i]->edges[j]->face1->index == i)
                    other = gi->game_grid->faces[i]->edges[j]->face2;
                else
                    other = gi->game_grid->faces[i]->edges[j]->face1;
                if (other == NULL)
                    continue;

                /* determine direction, outwards normal */
                int nx = gi->game_grid->faces[i]->dots[(j+1)%order]->y
                    - gi->game_grid->faces[i]->dots[j]->y;
                int ny = gi->game_grid->faces[i]->dots[j]->x
                    - gi->game_grid->faces[i]->dots[(j+1)%order]->x;
                if (nx > abs(ny))
                    cur_right[i] = other->index;
                else if (nx < -abs(ny))
                    cur_left[i] = other->index;
                else if (ny > abs(nx))
                    cur_down[i] = other->index;
                else if (ny < -abs(nx))
                    cur_up[i] = other->index;
                else {
                    /* exactly 45 degrees, go nowhere */
                }
            }
        }
        break;
    default:
        gi->has_cursor = false;
        for (i=0; i < gi->ntiles; i++) {
            cur_right[i] = -1;
            cur_left[i] = -1;
            cur_up[i] = -1;
            cur_down[i] = -1;
        }
    }

#ifdef GRID_DIAGNOSTICS
    {
        for (i = 0; i < gi->ntiles; i++) {
            if (cur_right[i] < -1 || cur_right[i] >= gi->ntiles)
                printf("Cursor array error cur_right[%d] = %d\n", i, cur_right[i]);
            if (cur_left[i] < -1 || cur_left[i] >= gi->ntiles)
                printf("Cursor array error cur_left[%d] = %d\n", i, cur_left[i]);
            if (cur_up[i] < -1 || cur_up[i] >= gi->ntiles)
                printf("Cursor array error cur_up[%d] = %d\n", i, cur_up[i]);
            if (cur_down[i] < -1 || cur_down[i] >= gi->ntiles)
                printf("Cursor array error cur_down[%d] = %d\n", i, cur_down[i]);
        }
    }
#endif

    gi->cursor_right = cur_right;
    gi->cursor_left = cur_left;
    gi->cursor_up = cur_up;
    gi->cursor_down = cur_down;
}

static struct grid_info *new_grid(const game_params *params, char *desc)
{
    int i, j, nrims;
    struct grid_info *ret = snew(struct grid_info);

    ret->refcount = 1;
    ret->game_grid = grid_new(grid_types[params->type],
                              params->w, params->h, desc);
    ret->type = params->type;
    ret->ntiles = ret->game_grid->num_faces;
    ret->tilesize = round(ret->game_grid->tilesize*grid_scale[params->type]);
    if (desc)
        ret->desc = dupstr(desc);
    else
        ret->desc = NULL;

#ifdef GRID_DIAGNOSTICS
    printf("Number of tiles: %d, w=%d, h=%d\n",
           ret->ntiles, params->w, params->h);
#endif

    /* Calculate all incentre */
    for (i = 0; i < ret->ntiles; i++)
        grid_find_incentre(ret->game_grid->faces[i]);

    update_neighbours(params, ret);
    update_cursor(params, ret);


    /*
     * We just allocate the tile_info array here.  It is filled in by
     * the drawing routines.
     */
    ret->tiles = snewn(ret->ntiles, struct tile_info);
    for (i = 0; i < ret->ntiles; i++) {
        ret->tiles[i].order = ret->game_grid->faces[i]->order;
        ret->tiles[i].coords = snewn(2*ret->game_grid->faces[i]->order, int);
    }

    /* Count outside edges */
    nrims = 0;
    for (i = 0; i < ret->game_grid->num_edges; i++)
        if (ret->game_grid->edges[i]->face1 == NULL ||
            ret->game_grid->edges[i]->face2 == NULL)
            nrims++;
    ret->nrims = nrims;

    ret->rims = snewn(nrims, struct rim_info);

    grid_edge *e = NULL;
    ret->rim_dots = snewn(nrims, grid_dot *);
    /* find an outer edge */
    for (i = 0; i < ret->game_grid->num_edges; i++) {
        if (ret->game_grid->edges[i]->face1 == NULL ||
            ret->game_grid->edges[i]->face2 == NULL) {
            e = ret->game_grid->edges[i];
            break;
        }
    }
    assert(e != NULL);

    grid_face *f;
    grid_dot *d;
    /* find out which side is outwards (needed for ligh or dark higlightning) */
    f = (e->face1 != NULL) ? e->face1 : e->face2;
    for (i = 0; i < f->order; i++) {
        if (f->dots[i] == e->dot1) {
            if (f->dots[(i+1)%f->order] == e->dot2) {
                d = e->dot2;
            } else {
                d = e->dot1;
            }
            break;
        }
    }
    for (i = 0; i < nrims; i++) {
        ret->rim_dots[i] = d;
        for (j = 0; j < d->order; j++) {
            if (d->edges[j] == e)
                continue;
            if (d->edges[j]->face1 == NULL || d->edges[j]->face2 == NULL) {
                e = d->edges[j];
                break;
            }
        }
        d = (d == e->dot1)? e->dot2 : e->dot1;
    }
    assert(d == ret->rim_dots[0]); /* This could happen if grid has a
                                    * hole, i.e., the rim is not just
                                    * one loop.
                                    */
    return ret;
}

static void free_grid_info(struct grid_info *gi)
{
    int i;

    sfree(gi->neighbours);
    sfree(gi->cursor_right);
    sfree(gi->cursor_left);
    sfree(gi->cursor_up);
    sfree(gi->cursor_down);
    for (i = 0; i < gi->ntiles; i++)
        sfree(gi->tiles[i].coords);
    sfree(gi->tiles);
    sfree(gi->rims);
    sfree(gi->rim_dots);
    sfree(gi->desc);
    grid_free(gi->game_grid);
    sfree(gi);
}

/* ----------------------------------------------------------------------
 * params handling routines and menues
 */

static game_params *default_params(void)
{
    game_params *ret = snew(game_params);

    ret->w = ret->h = 9;
    ret->n = 10;
    ret->type = MINES_GRID_SQUARE;
    ret->unique = true;
    ret->first_click_tile = -1;

    return ret;
}

static const struct game_params mines_presets_top[] = {
    {9, 9, 10, MINES_GRID_SQUARE, true, -1},         /* 81 tiles, 12.3% */
    {9, 9, 35, MINES_GRID_SQUARE, true, -1},         /* 81 tiles, 43.2% */
    {16, 16, 40, MINES_GRID_SQUARE, true, -1},       /* 256 tiles, 15.6% */
    {16, 16, 99, MINES_GRID_SQUARE, true, -1},       /* 256 tiles, 38.7% */
#ifndef SMALL_SCREEN
    {30, 16, 99, MINES_GRID_SQUARE, true, -1},       /* 480 tiles, 20.6% */
    {30, 16, 170, MINES_GRID_SQUARE, true, -1},      /* 480 tiles, 35.4% */
#endif
    {9, 9, 20, MINES_GRID_SQUARE_CYCLIC, true, -1},  /* 81 tiles, 24.7% */
    {10, 10, 20, MINES_GRID_HONEYCOMB, true, -1},    /* 100 tiles, 10.0% */
    {8, 8, 15, MINES_GRID_HONEYCOMB_CYCLIC, true, -1}, /* 64 tiles, 23.4% */
    {7, 7, 34, MINES_GRID_TRIANGULAR, true, -1},     /* 103 tiles, 33.0% */
#ifndef SMALL_SCREEN
    {10, 10, 70, MINES_GRID_TRIANGULAR, true, -1},   /* 210 tiles, 33.3% */
#endif
    {5, 6, 10, MINES_GRID_TRIANGULAR_CYCLIC, true, -1}, /* 60 tiles, 33.3% */
};

static const struct game_params mines_presets_more_small[] = {
    {9, 9, 10, MINES_GRID_OCTAGONAL2, true, -1},     /* 81 tiles, 12.3% */
    {5, 5, 20, MINES_GRID_SNUBSQUARE, true, -1},     /* 65 tiles, 30.8% */
    {7, 7, 17, MINES_GRID_CAIRO, true, -1},          /* 84 tiles, 20.2% */
    {4, 4, 15, MINES_GRID_KITE, true, -1},           /* 96 tiles, 15.6% */
    {5, 4, 20, MINES_GRID_GREATHEXAGONAL, true, -1}, /* 87 tiles, 23.0% */
    {5, 4, 15, MINES_GRID_KAGOME, true, -1},         /* 58 tiles, 25.9% */
    {4, 4, 15, MINES_GRID_FLORET, true, -1},         /* 84 tiles, 17.9% */
#ifndef SMALL_SCREEN
    {5, 4, 20, MINES_GRID_GREATDODECAGONAL, true, -1}, /* 87 tiles, 23.0% */
#endif
    {3, 2, 10, MINES_GRID_GREATGREATDODECAGONAL, true, -1}, /* 31 tiles, 32.3% */
    { 9, 9, 10, MINES_GRID_PENROSE_P3, true, -1},   /* 38--51 tiles, 19.6--26.3% */
    {10, 10, 15, MINES_GRID_PENROSE_P2, true, -1},   /* 60--64 tiles, 23.4--25.0% */
#ifdef SMALL_SCREEN
    {8, 8, 10, MINES_GRID_HATS, true, -1},           /* 30--36 tiles, 27.8--33.3% */
    {8, 8, 10, MINES_GRID_SPECTRES, true, -1},       /* 26--31 tiles, 32.3--38.5% */
#else
    {10, 10, 20, MINES_GRID_HATS, true, -1},         /* 54--59 tiles, 33.9--37.0% */
    {10, 10, 15, MINES_GRID_SPECTRES, true, -1},     /* 50--60 tiles, 22.2--26.8% */
#endif
};

#ifndef SMALL_SCREEN
static const struct game_params mines_presets_more_big[] = {
    {16, 16, 40, MINES_GRID_OCTAGONAL2, true, -1},   /* 256 tiles, 15.6% */
    {30, 16, 99, MINES_GRID_OCTAGONAL2, true, -1},   /* 480 tiles, 20.6% */
    {30, 16, 150, MINES_GRID_OCTAGONAL2, true, -1},  /* 480 tiles, 31.2% */
    {7, 7, 40, MINES_GRID_SNUBSQUARE, true, -1},     /* 133 tiles, 30.1% */
    {9, 9, 30, MINES_GRID_CAIRO, true, -1},          /* 144 tiles, 20.8% */
    {9, 7, 40, MINES_GRID_KITE, true, -1},           /* 378 tiles, 10.6% */
    {9, 5, 40, MINES_GRID_GREATHEXAGONAL, true, -1}, /* 217 tiles, 18.4% */
    {7, 6, 20, MINES_GRID_KAGOME, true, -1},         /* 124 tiles, 16.1% */
    {5, 5, 20, MINES_GRID_FLORET, true, -1},         /* 132 tiles, 15.2% */
    {7, 6, 60, MINES_GRID_GREATDODECAGONAL, true, -1}, /* 203 tiles, 29.6% */
    {5, 3, 20, MINES_GRID_GREATGREATDODECAGONAL, true, -1}, /* 109 tiles, 18.3% */
    {16, 16, 30, MINES_GRID_PENROSE_P3, true, -1},   /* 158--174 tiles, 17.2--19.0% */
    {16, 16, 30, MINES_GRID_PENROSE_P2, true, -1},   /* 176--190 tiles, 15.8--17.0% */
    {16, 16, 60, MINES_GRID_HATS, true, -1},         /* 159--167 tiles, 35.9--37.7% */
    {16, 16, 50, MINES_GRID_SPECTRES, true, -1},     /* 150--160 tiles, 31.2--33.3% */
};
#endif

/*
 * Dodecagonal and Comapss-Dodecagonal are not included in the
 * presets.  The difference in size on the tiles make them less good
 * for Mines.
 *
 * There is no SMALL_SCREEN version of Great-Dodecagonal.  Just click
 * on a large tile in the middle and you are almost done.
 * Great-great-dodecagonal have the same problem but to a lesser
 * degree so it is included anyway.
 */

static void preset_menu_add_preset_with_title(struct preset_menu *menu,
                                              const game_params *params)
{
    char buf[80];
    game_params *dup_params;

    sprintf(buf, "%dx%d, %d mines, %s",
            params->w, params->h, params->n, gridnames[params->type]);

    dup_params = snew(game_params);
    *dup_params = *params;

    preset_menu_add_preset(menu, dupstr(buf), dup_params);
}



static struct preset_menu * game_preset_menu(void)
{
    struct preset_menu *top, *more;
    int i;

    top = preset_menu_new();

    for (i = 0; i < lenof(mines_presets_top); i++)
        preset_menu_add_preset_with_title(top, &mines_presets_top[i]);

#ifdef SMALL_SCREEN
    const char *menutxt = "More";
#else
    const char *menutxt = "More (small)";
#endif
    more = preset_menu_add_submenu(top, dupstr(menutxt));
    for (i = 0; i < lenof(mines_presets_more_small); i++)
        preset_menu_add_preset_with_title(more, &mines_presets_more_small[i]);

#ifndef SMALL_SCREEN
    struct preset_menu *big;
    big = preset_menu_add_submenu(top, dupstr("More"));
    for (i = 0; i < lenof(mines_presets_more_big); i++)
        preset_menu_add_preset_with_title(big, &mines_presets_more_big[i]);
#endif

    return top;
}

static void free_params(game_params *params)
{
    sfree(params);
}

static game_params *dup_params(const game_params *params)
{
    game_params *ret = snew(game_params);
    *ret = *params;		       /* structure copy */
    return ret;
}

static void decode_params(game_params *params, char const *string)
{
    char const *p = string;
    int x = -1, y = -1;

    params->w = atoi(p);
    while (*p && isdigit((unsigned char)*p)) p++;
    if (*p == 'x') {
        p++;
        params->h = atoi(p);
        while (*p && isdigit((unsigned char)*p)) p++;
    } else {
        params->h = params->w;
    }
    if (*p == 't') {
        p++;
        params->type = atoi(p);
	while (*p && (isdigit((unsigned char)*p))) p++;
    } else {
        params->type = MINES_GRID_SQUARE;
    }
    if (*p == 'n') {
	p++;
	params->n = atoi(p);
	while (*p && (*p == '.' || isdigit((unsigned char)*p))) p++;
    } else {
        if (params->h > 0 && params->w > 0 &&
            params->w <= INT_MAX / params->h)
            params->n = params->w * params->h / 10;
    }

    while (*p) {
	if (*p == 'a') {
            p++;
	    params->unique = false;
	} else if (*p == 'X') {
            p++;
            x = atoi(p);
            while (*p && isdigit((unsigned char)*p)) p++;
	} else if (*p == 'Y') {
            p++;
            y = atoi(p);
            while (*p && isdigit((unsigned char)*p)) p++;
	} else if (*p == 'I') {
            p++;
            params->first_click_tile = atoi(p);
            while (*p && isdigit((unsigned char)*p)) p++;
	} else
	    p++;		       /* skip any other gunk */
    }
    if (x >= 0 && y >= 0)
        /* This can only happen for the square grid. */
        params->first_click_tile = y*params->w + x;
}

static char *encode_params(const game_params *params, bool full)
{
    char ret[400];
    int len;

    len = sprintf(ret, "%dx%d", params->w, params->h);
    if (params->type != MINES_GRID_SQUARE)
        len += sprintf(ret+len, "t%d", params->type);
    /*
     * Mine count is a generation-time parameter, since it can be
     * deduced from the mine bitmap!
     */
    if (full)
	len += sprintf(ret+len, "n%d", params->n);
    if (full && !params->unique)
        ret[len++] = 'a';
    if (full && params->first_click_tile >= 0) {
        if (params->type == MINES_GRID_SQUARE) {
            int x = params->first_click_tile % params->w;
            int y = params->first_click_tile / params->w;
            len += sprintf(ret+len, "X%d", x);
            len += sprintf(ret+len, "Y%d", y);
        } else {
            len += sprintf(ret+len, "I%d", params->first_click_tile);
        }
    }
    assert(len < lenof(ret));
    ret[len] = '\0';

    return dupstr(ret);
}

static config_item *game_configure(const game_params *params)
{
    config_item *ret;
    char buf[80];

    ret = snewn(6, config_item);

    ret[0].name = "Width";
    ret[0].type = C_STRING;
    sprintf(buf, "%d", params->w);
    ret[0].u.string.sval = dupstr(buf);

    ret[1].name = "Height";
    ret[1].type = C_STRING;
    sprintf(buf, "%d", params->h);
    ret[1].u.string.sval = dupstr(buf);

    ret[2].name = "Mines";
    ret[2].type = C_STRING;
    sprintf(buf, "%d", params->n);
    ret[2].u.string.sval = dupstr(buf);

    ret[3].name = "Grid type";
    ret[3].type = C_CHOICES;
    ret[3].u.choices.choicenames = GRID_CONFIGS;
    ret[3].u.choices.selected = params->type;

    ret[4].name = "Ensure solubility";
    ret[4].type = C_BOOLEAN;
    ret[4].u.boolean.bval = params->unique;

    ret[5].name = NULL;
    ret[5].type = C_END;

    return ret;
}

static game_params *custom_params(const config_item *cfg)
{
    game_params *ret = snew(game_params);

    ret->w = atoi(cfg[0].u.string.sval);
    ret->h = atoi(cfg[1].u.string.sval);
    ret->n = atoi(cfg[2].u.string.sval);
    if (strchr(cfg[2].u.string.sval, '%'))
	ret->n = ret->n * (ret->w * ret->h) / 100;
    ret->type = cfg[3].u.choices.selected;
    ret->unique = cfg[4].u.boolean.bval;
    ret->first_click_tile = -1;

    return ret;
}


static const char *validate_params(const game_params *params, bool full)
{
    int whmin = min(params->w, params->h);
    int whmax = max(params->w, params->h);
    const char *err;

    /* General constraints for all grids. */
    
    if (params->type < 0 || params->type >= NUM_GRID_TYPES)
        return "Illegal grid type";

    if (params->w < 0 || params->h < 0)
        return "Width and height may not be negative";
    if (params->w == 0 || params->h == 0)
        return "Width and height may not be zero";
    if (whmin < 0)
        return "Width and height must not be negative";
    if (whmin < 1)
        return "Width and height must not be zero";
    if (params->n < 0)
	return "Mine count may not be negative";
    if (params->n < 1)
        return "Number of mines must be greater than zero";

    err = grid_validate_params(grid_types[params->type], params->w, params->h);
    if (err != NULL) return err;

    /* Grid specific constraints */
    switch (params->type) {
    case (MINES_GRID_SQUARE):
        /*
         * Lower limit on grid size: each dimension must be at least 3.
         * 1 is theoretically workable if rather boring, but 2 is a
         * real problem: there is often _no_ way to generate a uniquely
         * solvable 2xn Mines grid. You either run into two mines
         * blocking the way and no idea what's behind them, or one mine
         * and no way to know which of the two rows it's in. If the
         * mine count is even you can create a soluble grid by packing
         * all the mines at one end (so that when you hit a two-mine
         * wall there are only as many covered squares left as there
         * are mines); but if it's odd, you are doomed, because you
         * _have_ to have a gap somewhere which you can't determine the
         * position of.
         */
        if (full && params->unique && whmin <= 2)
            return "Width and height must both be greater than two for this grid.";
        if (whmax < 4)
            return "At least one of width and height must be larger than three";
        break;
    case MINES_GRID_SQUARE_CYCLIC:
        if (full && params->unique && (params->w <= 3 || params->h <= 3))
            return "Width and height must both be greater than three for this grid";
        if (whmax < 4)
            return "At least one of width and height must be larger than three";
        break;
    case MINES_GRID_HONEYCOMB:
        break;
    case MINES_GRID_HONEYCOMB_CYCLIC:
        if (params->w % 2 == 1)
            return "Width must be even for this grid";
        if (full && params->unique && params->h == 2)
            return "Height may not be two for this grid";
        break;
    case MINES_GRID_OCTAGONAL2:
        break;
    case MINES_GRID_TRIANGULAR:
        if (full && params->unique && params->h == 4 && params->w == 1)
            return "Grid too small for unique solution";
        break;
    case MINES_GRID_TRIANGULAR_CYCLIC:
        if (params->h % 2 == 1)
            return "Height must be even for this grid";
        if (full && params->unique && params->w == 1)
            return "Width must be larger than one for unique solution";
        break;
    case MINES_GRID_SNUBSQUARE:
        if (full && params->unique &&
            (params->w == 3 && params->h == 1) &&
            (params->w == 4 && params->h == 1) &&
            (params->w == 1 && params->h == 3))
            return "Grid too small for unique solution";
        break;
    case MINES_GRID_CAIRO:
        if (whmin < 2)
            return "Width and height must be at least two for this grid";
        break;
    case MINES_GRID_KITE:
        if (full && params->unique && whmin == 1)
            return "Width and height must be at least two for unique solution for this grid";
        break;
    case MINES_GRID_GREATHEXAGONAL:
        break;
    case MINES_GRID_KAGOME:
        if (full && params->unique && params->h == 1)
            return "Height must be at least two for unique solution for this grid";
        break;
    case MINES_GRID_FLORET:
        if (full && params->unique &&
            ( (params->w == 1 && params->h == 2) ||
              (params->w == 2 && params->h == 1)))
            return "Grid size too small for unique solution for this grid";
        break;
    case MINES_GRID_DODECAGONAL:
        if (full && params->unique && (params->w == 3 && params->h == 3))
            return "Grid size too small for unique solution for this grid";
        break;
    case MINES_GRID_GREATDODECAGONAL:
        break;
    case MINES_GRID_GREATGREATDODECAGONAL:
        break;
    case MINES_GRID_COMPASSDODECAGONAL:
        break;
    case MINES_GRID_PENROSE_P3:
        /*
         * This grid depends on random numbers during grid generation.
         * Some times it comes out well and some times not.  5x5 fails
         * to give a good grid rather often.
         */
        if (full && whmin < 6)
            return "Width and height must both be larger than 5 for this grid";

        break;
    case MINES_GRID_HATS:
        /*
         * This grid depends on random numbers during grid
         * generation. 5x4 often fails to give a playable grid.  5x5
         * seems to be safe.
         */
        if (full && whmin < 5)
            return "Width and height must both be larger than 4 for this grid";

        break;
    case MINES_GRID_PENROSE_P2:
        /*
         * This grid depends on random numbers during grid
         * generation. 5x4 often fails to give a playable grid.  5x5
         * usually workds.
         */
        if (full && whmin < 5)
            return "Width and height must both be larger than 4 for this grid";

        break;
    case MINES_GRID_SPECTRES:
        /*
         * This grid depends on random numbers during grid
         * generation. 5x5 often fails to give a playable grid.  6x5
         * fails rarely.  6x6 seems safe.
         */
        if (full && whmin < 6)
            return "Width and height must both be larger than 5 for this grid";

        break;
    }

    /*
     * FIXME: Need more constraints here. Not sure what the
     * sensible limits for Minesweeper actually are. The limits
     * probably ought to change, however, depending on uniqueness.
     */

    return NULL;
}

/* ----------------------------------------------------------------------
 * Minesweeper solver, used to ensure the generated grids are
 * solvable without having to take risks.
 */

static bool is_neighbour_of(struct grid_info *gi, int tile1, int tile2)
{
    int i;

    if (tile1 < 0 || tile2 < 0)
        return false;

    for (i = tile1*gi->nsize; i < (tile1+1)*gi->nsize; i++)
        if (tile2 == gi->neighbours[i])
            return true;
    return false;
}

/* ----------------------------------------------------------------------
 * Sets and todo list
 */

/*
 * We store a number of small localised sets in a double linked list,
 * each with a mine count. We also keep some of those sets linked
 * together into a to-do list.
 */

struct set {
    int tile[MAX_NEIGHBOURS];
    int n;  /* Number of items in this set. */
    short mines;
    bool todo;
    struct set *todo_prev, *todo_next;  /* todo list */
    struct set *all_prev, *all_next; /* All sets in store */
};

struct setstore {
    int n;  /* Number of sets in all-store */
    struct set *all_head, *all_tail;
    struct set *todo_head, *todo_tail;
};

#ifdef SOLVER_DIAGNOSTICS
static void setprint(struct set *s)
{
    int i;
    for (i = 0; i < s->n; i++)
        printf("%d ", s->tile[i]);
    printf("mines: %d\n", s->mines);
}
#endif

static struct setstore *ss_new(void)
{
    struct setstore *ss = snew(struct setstore);
    ss->todo_head = ss->todo_tail = NULL;
    ss->all_head = ss->all_tail = NULL;
    ss->n = 0;
    return ss;
}

static struct set * set_new(void) {
    struct set *s = snew(struct set);
    s->n = 0;
    s->todo_next = s->todo_prev = NULL;
    s->all_next = s->all_prev = NULL;
    s->mines = 0;
    s->todo = false;
    return s;
}

static struct set * set_dup(struct set *s) {
    int i;
    struct set *ret = snew(struct set);
    ret->n = s->n;
    ret->todo_next = ret->todo_prev = NULL;
    ret->all_next = ret->all_prev = NULL;
    ret->mines = s->mines;
    ret->todo = false;
    for (i = 0; i < s->n; i++)
        ret->tile[i] = s->tile[i];
    return ret;
}

/*
 * Remove tile from set.
 * Does not update the mine count.
 */
static void set_remove_tile(struct set *s, int tile)
{
    int i, j;
    for (i = 0; i < s->n; i++) {
        if (s->tile[i] == tile) {
            for (j = i; j < s->n-1; j++)
                s->tile[j] = s->tile[j+1];
            s->n--;
            return;
        }
    }
    /* We should never come here! */
    assert(!"Tried to remove non-present tile from set");
}

/*
 * Take two input sets, a and b, and calculate its wings, i.e. a-b and b-a.
 * The output wings are assumed to be empty at start of the function.
 */
static void setwings(struct set *a, struct set *b,
              struct set *wing1, struct set *wing2)
{
    int i = 0, j = 0;

    wing1->n = 0;
    wing2->n = 0;
    while (i < a->n && j < b->n) {
        if (a->tile[i] < b->tile[j])
            wing1->tile[wing1->n++] = a->tile[i++];
        else if (a->tile[i] > b->tile[j])
            wing2->tile[wing2->n++] = b->tile[j++];
        else
            i++, j++;
    }

    while (i < a->n)
        wing1->tile[wing1->n++] = a->tile[i++];

    while (j < b->n)
        wing2->tile[wing2->n++] = b->tile[j++];
}

/*
 * Take two input sets, and return true if they overlap.
 */
static bool setoverlap(struct set *a, struct set *b)
{
    int i = 0, j = 0;
    while(i < a->n && j < b->n) {
        if (a->tile[i] == b->tile[j])
            return true;
        if (a->tile[i] < b->tile[j])
            i++;
        else
            j++;
    }
    return false;
}

/*
 * Check if a set contains a given tile.
 */
static bool setcontains(struct set *s, int tile)
{
    int i;
    for (i = 0; i < s->n; i++)
        if (s->tile[i] == tile)
            return true;

    return false;
}

static void ss_add_todo(struct setstore *ss, struct set *s)
{
    if (s->todo)
	return;			       /* already on it */

#ifdef SOLVER_DIAGNOSTICS
    printf("adding set on todo list: ");
    setprint(s);
#endif

    s->todo_prev = ss->todo_tail;
    if (s->todo_prev)
	s->todo_prev->todo_next = s;
    else
	ss->todo_head = s;
    ss->todo_tail = s;
    s->todo_next = NULL;
    s->todo = true;
}

static int intcmp(const void *a, const void *b)
{
    int ia = *((int *)a);
    int ib = *((int *)b);
    return (ia < ib) ? -1 : +1;  /* Equal should not happen */
}

static void ss_add(struct setstore *ss, struct set *s)
{
    struct set *find;

    /*
     * Add a set to the store.
     */

    /* First sort it */
    qsort(s->tile, s->n, sizeof(int), intcmp);

    /* Check if it exist already */
    for (find = ss->all_head; find; find = find->all_next) {
        if (find->n != s->n)
            continue;
        int i;
        bool differ = false;
        for (i = 0; i < s->n; i++) {
            if (find->tile[i] != s->tile[i]) {
                differ = true;
                break;
            }
        }
        if (!differ) {
            /*
             * This set already existed! Free it and return.
             */
            sfree(s);
            return;
        }
    }

    s->all_next = NULL;
    s->all_prev = ss->all_tail;
    if (s->all_prev)
        s->all_prev->all_next = s;
    ss->all_tail = s;
    if (!ss->all_head)
        ss->all_head = s;
    ss->n++;

    /*
     * We've added a new set to the all-list, so put it on the todo
     * list.
     */
    ss_add_todo(ss, s);
}

static void ss_remove(struct setstore *ss, struct set *s)
{
    struct set *next = s->todo_next, *prev = s->todo_prev;

#ifdef SOLVER_DIAGNOSTICS
    printf("removing set ");
    setprint(s);
#endif
    /*
     * Remove s from the todo list.
     */
    if (prev)
	prev->todo_next = next;
    else if (s == ss->todo_head)
	ss->todo_head = next;

    if (next)
	next->todo_prev = prev;
    else if (s == ss->todo_tail)
	ss->todo_tail = prev;

    s->todo = false;

    /*
     * Remove s from the all list.
     */
    next = s->all_next; prev = s->all_prev;
    if (prev)
	prev->all_next = next;
    else if (s == ss->all_head)
	ss->all_head = next;

    if (next)
	next->all_prev = prev;
    else if (s == ss->all_tail)
	ss->all_tail = prev;

    ss->n--;

    /*
     * Destroy the actual set structure.
     */
    sfree(s);
}

/*
 * Check if a set already in the store is a copy of another set.  If
 * so, remove it, otherwise add it to the todo-list if it is not
 * already added.
 */
static void ss_check_uniq(struct setstore *ss, struct set *s)
{
    int i;
    struct set *find;

    for (find = ss->all_head; find; find = find->all_next) {
        if (find == s || find->n != s->n)
            continue;
        bool equal = true;
        for ( i = 0; i < s->n; i++)
            if (find->tile[i] != s->tile[i]) {
                equal = false;
                break;
            }
        if (equal) {
            /* They are equal */
            ss_remove(ss, s);
            return;
        }
    }
    /* If we gets here, it means the set is uniq.  Add it to the todo-list. */
    if (!s->todo)
        ss_add_todo(ss, s);
}

/*
 * Return a dynamically allocated list of all the sets which
 * contains a provided tile.
 */
static struct set **ss_contains(struct setstore *ss, int tile)
{
    struct set **ret = NULL;
    int nret = 0, retsize = 0;
    struct set *find;

    for (find = ss->all_head; find; find = find->all_next) {
        int i;
        for (i = 0; i < find->n; i++) {
            if (find->tile[i] == tile) {
                /*
                 * Found the tile.
                 */
                if (nret >= retsize) {
                    retsize = nret + 32;
                    ret = sresize(ret, retsize, struct set *);
                }
                ret[nret++] = find;

                break;
            }
        }
    }
    ret = sresize(ret, nret+1, struct set *);
    ret[nret] = NULL;

    return ret;
}

/*
 * Return a dynamically allocated list of all the sets which
 * overlap a provided input set.
 */
static struct set **ss_overlap(struct setstore *ss, struct set * s)
{
    struct set **ret = NULL;
    int nret = 0, retsize = 0;

    struct set *find;

    for (find = ss->all_head; find; find = find->all_next) {
        int i = 0, j = 0;
        while (i < s->n && j < find->n) {
            if (s->tile[i] == find->tile[j]) {
                /*
                 * There's an overlap.
                 */
                if (nret >= retsize) {
                    retsize = nret + 32;
                    ret = sresize(ret, retsize, struct set *);
                }
                ret[nret++] = find;

                break;
            }
            if (s->tile[i] < find->tile[j])
                i++;
            else
                j++;
        }
    }

    ret = sresize(ret, nret+1, struct set *);
    ret[nret] = NULL;

    return ret;
}

/*
 * Get an element from the head of the set todo list.
 */
static struct set *ss_todo(struct setstore *ss)
{
    if (ss->todo_head) {
	struct set *ret = ss->todo_head;
	ss->todo_head = ret->todo_next;
	if (ss->todo_head)
	    ss->todo_head->todo_prev = NULL;
	else
	    ss->todo_tail = NULL;
	ret->todo_next = ret->todo_prev = NULL;
	ret->todo = false;
	return ret;
    } else {
	return NULL;
    }
}

struct squaretodo {
    int *next;
    int head, tail;
};

static void std_add(struct squaretodo *std, int i)
{
    if (std->tail >= 0)
	std->next[std->tail] = i;
    else
	std->head = i;
    std->tail = i;
    std->next[i] = -1;
}

typedef int (*open_cb)(void *, int);

static void known_squares(struct squaretodo *std,
                          signed char *board,
			  open_cb open, void *openctx,
			  struct set *s, bool mine)
{
    int i, tile;

    for (i = 0; i < s->n; i++) {
        /*
         * It's possible that this square is _already_
         * known, in which case we don't try to add it to
         * the list twice.
         */
        tile = s->tile[i];
        if (board[tile] == -2) {
            if (mine) {
                board[tile] = -1;   /* and don't open it! */
            } else {
                board[tile] = open(openctx, tile);
                assert(board[tile] != -1);   /* *bang* */
            }
            std_add(std, tile);
        }
    }
}

/* ----------------------------------------------------------------------
 * Solver
 */

/*
 * This is data returned from the `perturb' function. It details
 * which squares have become mines and which have become clear. The
 * solver is (of course) expected to honourably not use that
 * knowledge directly, but to efficently adjust its internal data
 * structures and proceed based on only the information it
 * legitimately has.
 */
struct perturbation {
    int tile;
    int delta;			       /* +1 == become a mine; -1 == cleared */
};
struct perturbations {
    int n;
    struct perturbation *changes;
};

/* Used for backwards compatibility with old Square mines */
struct setsort {
    struct set *this;
    int tile;  /* Virtual upper left tile */
    short mask;
};

/* Used for backwards compatibility with old Square mines */
static int setsort_cmp(const void *arg_a, const void *arg_b)
{
    struct setsort *a = (struct setsort *)arg_a;
    struct setsort *b = (struct setsort *)arg_b;

    if (a->tile < b->tile)   return -1;
    if (a->tile > b->tile)   return +1;
    if (a->mask < b->mask) return -1;
    if (a->mask > b->mask) return +1;
    return 0;
}

/*
 * Main solver entry point. You give it a board of existing
 * knowledge (-1 for a square known to be a mine, 0-MAX_NEIGHBOURS for empty
 * squares with a given number of neighbours, -2 for completely
 * unknown), plus a function which you can call to open new squares
 * once you're confident of them. It fills in as much more of the
 * board as it can.
 * 
 * Return value is:
 * 
 *  - -1 means deduction stalled and nothing could be done
 *  - 0 means deduction succeeded fully
 *  - >0 means deduction succeeded but some number of perturbation
 *    steps were required; the exact return value is the number of
 *    perturb calls.
 */

typedef struct perturbations *(*perturb_cb) (void *, signed char *, struct set *);

static int minesolve(struct grid_info *gi, int w, int h, int n, signed char *board,
		     open_cb open,
                     perturb_cb perturb,
		     void *ctx, random_state *rs)
{
    struct setstore *ss = ss_new();
    struct set **list;
    struct squaretodo astd, *std = &astd;
    int i, j;
    int tile;
    int nperturbs = 0;

    /*
     * Set up a linked list of squares with known contents, so that
     * we can process them one by one.
     */
    std->next = snewn(gi->ntiles, int);
    std->head = std->tail = -1;

    /*
     * Initialise that list with all known squares in the input
     * board.
     */
    for (i = 0; i < gi->ntiles; i++)
        if (board[i] != -2)
            std_add(std, i);

    /*
     * Main deductive loop.
     */
    while (1) {
	bool done_something = false;
	struct set *s;

	/*
	 * If there are any known squares on the todo list, process
	 * them and construct a set for each.
	 */
	while (std->head != -1) {
	    tile = std->head;
#ifdef SOLVER_DIAGNOSTICS
	    printf("known square at %d [%d]\n", tile, board[tile]);
#endif
	    std->head = std->next[tile];
	    if (std->head == -1)
		std->tail = -1;

	    if (board[tile] >= 0) {
		int mines, ni;
#ifdef SOLVER_DIAGNOSTICS
		printf("creating set around this square\n");
#endif
		/*
		 * Empty square. Construct the set of non-known squares
		 * around this one, and determine its mine count.
		 */
		mines = board[tile];
                s = set_new();
                for (j = 0; j < gi->nsize; j++) {
                    ni = gi->neighbours[tile*gi->nsize + j];
                    if ( ni >= 0 ) {
#ifdef SOLVER_DIAGNOSTICS
                        printf("board %d = %d\n", ni, board[ni]);
#endif
                        if (board[ni] == -1)
                            mines--;
                        else if (board[ni] == -2)
                            s->tile[s->n++] = ni;
                    }
		}
		if (s->n > 0) {
                    s->mines = mines;
		    ss_add(ss, s);
                } else {
                    sfree(s);
                }
	    }

	    /*
	     * Now, whether the square is empty or full, we must
	     * find any set which contains it and replace it with
	     * one which does not.
	     */
	    {
#ifdef SOLVER_DIAGNOSTICS
		printf("finding sets containing known square %d\n", tile);
#endif
		list = ss_contains(ss, tile);

		for (j = 0; list[j]; j++) {
		    s = list[j];

		    /*
		     * Remove the newly known square.
		     */
		    set_remove_tile(s, tile);

		    /*
		     * Compute the new mine count.
		     */
                    if (board[tile] == -1)
                        s->mines--;

		    /*
		     * Check if the modified set is already on the
		     * store or is empty.  Remove it if it is.
		     */

		    if (s->n == 0)
			ss_remove(ss, s);
                    else
                        ss_check_uniq(ss, s);

		}

		sfree(list);
	    }

	    /*
	     * Marking a fresh square as known certainly counts as
	     * doing something.
	     */
	    done_something = true;
	}

	/*
	 * Now pick a set off the to-do list and attempt deductions
	 * based on it.
	 */
	if ((s = ss_todo(ss)) != NULL) {

#ifdef SOLVER_DIAGNOSTICS
            printf("set to do: ");
            setprint(s);
#endif
	    /*
	     * Firstly, see if this set has a mine count of zero or
	     * of its own cardinality.
	     */
	    if (s->mines == 0 || s->mines == s->n) {
		/*
		 * If so, we can immediately mark all the squares
		 * in the set as known.
		 */
#ifdef SOLVER_DIAGNOSTICS
		printf("easy\n");
#endif
		known_squares(std, board, open, ctx, s, (s->mines != 0));

		/*
		 * Having done that, we need do nothing further
		 * with this set; marking all the squares in it as
		 * known will eventually eliminate it, and will
		 * also permit further deductions about anything
		 * that overlaps it.
		 */
		continue;
	    }

	    /*
	     * Failing that, we now search through all the sets
	     * which overlap this one.
	     */
	    list = ss_overlap(ss, s);

	    for (j = 0; list[j]; j++) {
		struct set *s2 = list[j];
                struct set wing1, wing2;

                if (s == s2)
                    continue;

		/*
		 * Find the non-overlapping parts s2-s and s-s2,
		 * and their cardinalities.
		 * 
		 * I'm going to refer to these parts as `wings'
		 * surrounding the central part common to both
		 * sets. The `s1 wing' is s-s2; the `s2 wing' is
		 * s2-s.
		 */
                setwings(s, s2, &wing1, &wing2);

		/*
		 * If one set has more mines than the other, and
		 * the number of extra mines is equal to the
		 * cardinality of that set's wing, then we can mark
		 * every square in the wing as a known mine, and
		 * every square in the other wing as known clear.
		 */
		if (wing1.n == s->mines - s2->mines ||
		    wing2.n == s2->mines - s->mines) {
		    known_squares(std, board, open, ctx,
				  &wing1, (wing1.n == s->mines - s2->mines));
		    known_squares(std, board, open, ctx,
				  &wing2, (wing2.n == s2->mines - s->mines));
		    continue;
		}

		/*
		 * Failing that, see if one set is a subset of the
		 * other. If so, we can divide up the mine count of
		 * the larger set between the smaller set and its
		 * complement, even if neither smaller set ends up
		 * being immediately clearable.
		 */
		if (wing1.n == 0 && wing2.n != 0) {
		    /* s is a subset of s2. */
		    assert(s2->mines > s->mines);
                    wing2.mines = s2->mines - s->mines;
		    ss_add(ss, set_dup(&wing2));
		} else if (wing2.n == 0 && wing1.n != 0) {
		    /* s2 is a subset of s. */
		    assert(s->mines > s2->mines);
                    wing1.mines = s->mines - s2->mines;
		    ss_add(ss, set_dup(&wing1));
		}
	    }

	    sfree(list);

	    /*
	     * In this situation we have definitely done
	     * _something_, even if it's only reducing the size of
	     * our to-do list.
	     */
	    done_something = true;
	} else if (n >= 0) {
	    /*
	     * We have nothing left on our todo list, which means
	     * all localised deductions have failed. Our next step
	     * is to resort to global deduction based on the total
	     * mine count. This is computationally expensive
	     * compared to any of the above deductions, which is
	     * why we only ever do it when all else fails, so that
	     * hopefully it won't have to happen too often.
	     * 
	     * If you pass n<0 into this solver, that informs it
	     * that you do not know the total mine count, so it
	     * won't even attempt these deductions.
	     */

	    int minesleft, squaresleft;
	    int nsets, cursor;
            bool setused[10];

	    /*
	     * Start by scanning the current board state to work out
	     * how many unknown squares we still have, and how many
	     * mines are to be placed in them.
	     */
	    squaresleft = 0;
	    minesleft = n;
	    for (i = 0; i < gi->ntiles; i++) {
		if (board[i] == -1)
		    minesleft--;
		else if (board[i] == -2)
		    squaresleft++;
	    }

#ifdef SOLVER_DIAGNOSTICS
	    printf("global deduction time: squaresleft=%d minesleft=%d\n",
		   squaresleft, minesleft);
            if (gi->type == MINES_GRID_SQUARE) {
                int x, y;
                for (y = 0; y < h; y++) {
                    for (x = 0; x < w; x++) {
                        int v = board[y*w+x];
                        if (v == -1)
                            putchar('*');
                        else if (v == -2)
                            putchar('?');
                        else if (v == 0)
                            putchar('-');
                        else
                            putchar('0' + v);
                    }
                    putchar('\n');
                }
            }
#endif

	    /*
	     * If there _are_ no unknown squares, we have actually
	     * finished.
	     */
	    if (squaresleft == 0) {
		assert(minesleft == 0);
		break;
	    }

	    /*
	     * First really simple case: if there are no more mines
	     * left, or if there are exactly as many mines left as
	     * squares to play them in, then it's all easy.
	     */
	    if (minesleft == 0 || minesleft == squaresleft) {
		for (i = 0; i < gi->ntiles; i++)
		    if (board[i] == -2) {
                        struct set s;
                        s.n = 1;
                        s.tile[0] = i;
			known_squares(std, board, open, ctx,
				      &s, minesleft != 0);
                    }
		continue;	       /* now go back to main deductive loop */
	    }

	    /*
	     * Failing that, we have to do some _real_ work.
	     * Ideally what we do here is to try every single
	     * combination of the currently available sets, in an
	     * attempt to find a disjoint union (i.e. a set of
	     * squares with a known mine count between them) such
	     * that the remaining unknown squares _not_ contained
	     * in that union either contain no mines or are all
	     * mines.
	     * 
	     * Actually enumerating all 2^n possibilities will get
	     * a bit slow for large n, so I artificially cap this
	     * recursion at n=10 to avoid too much pain.
	     */
            nsets = ss->n;
	    if (nsets <= lenof(setused)) {
		/*
		 * Doing this with actual recursive function calls
		 * would get fiddly because a load of local
		 * variables from this function would have to be
		 * passed down through the recursion. So instead
		 * I'm going to use a virtual recursion within this
		 * function. The way this works is:
		 * 
		 *  - we have an array `setused', such that setused[n]
		 *    is true if set n is currently in the union we
		 *    are considering.
		 * 
		 *  - we have a value `cursor' which indicates how
		 *    much of `setused' we have so far filled in.
		 *    It's conceptually the recursion depth.
		 * 
		 * We begin by setting `cursor' to zero. Then:
		 * 
		 *  - if cursor can advance, we advance it by one. We
		 *    set the value in `setused' that it went past to
		 *    true if that set is disjoint from anything else
		 *    currently in `setused', or to false otherwise.
		 * 
		 *  - If cursor cannot advance because it has
		 *    reached the end of the setused list, then we
		 *    have a maximal disjoint union. Check to see
		 *    whether its mine count has any useful
		 *    properties. If so, mark all the squares not
		 *    in the union as known and terminate.
		 * 
		 *  - If cursor has reached the end of setused and the
		 *    algorithm _hasn't_ terminated, back cursor up to
		 *    the nearest true entry, reset it to false, and
		 *    advance cursor just past it.
		 * 
		 *  - If we attempt to back up to the nearest 1 and
		 *    there isn't one at all, then we have gone
		 *    through all disjoint unions of sets in the
		 *    list and none of them has been helpful, so we
		 *    give up.
		 */
		struct set *sets[lenof(setused)];
                struct set *s = ss->all_head;
		for (i = 0; i < nsets; i++) {
		    sets[i] = s;
                    s = s->all_next;
                }

		cursor = 0;
		while (1) {

		    if (cursor < nsets) {
			bool ok = true;

			/* See if any existing set overlaps this one. */
			for (i = 0; i < cursor; i++)
			    if (setused[i] &&
                                setoverlap(sets[cursor], sets[i])) {
				ok = false;
				break;
			    }

			if (ok) {
			    /*
			     * We're adding this set to our union,
			     * so adjust minesleft and squaresleft
			     * appropriately.
			     */
			    minesleft -= sets[cursor]->mines;
			    squaresleft -= sets[cursor]->n;
			}

			setused[cursor++] = ok;
		    } else {
#ifdef SOLVER_DIAGNOSTICS
			printf("trying a set combination with %d %d\n",
			       squaresleft, minesleft);
#endif /* SOLVER_DIAGNOSTICS */

			/*
			 * We've reached the end. See if we've got
			 * anything interesting.
			 */
			if (squaresleft > 0 &&
			    (minesleft == 0 || minesleft == squaresleft)) {
			    /*
			     * We have! There is at least one
			     * square not contained within the set
			     * union we've just found, and we can
			     * deduce that either all such squares
			     * are mines or all are not (depending
			     * on whether minesleft==0). So now all
			     * we have to do is actually go through
			     * the board, find those squares, and
			     * mark them.
			     */
			    for (i = 0; i < gi->ntiles; i++)
				if (board[i] == -2) {
                                    bool outside = true;
				    for (j = 0; j < nsets; j++)
					if (setused[j] &&
					    setcontains(sets[j], i)) {
					    outside = false;
					    break;
					}
				    if (outside) {
                                        struct set s;
                                        s.tile[0] = i;
                                        s.n = 1;
					known_squares(std, board,
						      open, ctx,
						      &s, minesleft != 0);
                                    }
                                }

			    done_something = true;
			    break;     /* return to main deductive loop */
			}

			/*
			 * If we reach here, then this union hasn't
			 * done us any good, so move on to the
			 * next. Backtrack cursor to the nearest 1,
			 * change it to a 0 and continue.
			 */
			while (--cursor >= 0 && !setused[cursor]);
			if (cursor >= 0) {
			    assert(setused[cursor]);

			    /*
			     * We're removing this set from our
			     * union, so re-increment minesleft and
			     * squaresleft.
			     */
			    minesleft += sets[cursor]->mines;
			    squaresleft += sets[cursor]->n;

			    setused[cursor++] = false;
			} else {
			    /*
			     * We've backtracked all the way to the
			     * start without finding a single 1,
			     * which means that our virtual
			     * recursion is complete and nothing
			     * helped.
			     */
			    break;
			}
		    }

		}
	    }
	}

	if (done_something)
	    continue;

#ifdef SOLVER_DIAGNOSTICS
	/*
	 * Dump the current known state of the board.
	 */
	printf("solver ran out of steam, ret=%d, board:\n", nperturbs);
        if (gi->type == MINES_GRID_SQUARE) {
            int x, y;
            for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                    int v = board[y*w+x];
                    if (v == -1)
                        putchar('*');
                    else if (v == -2)
                        putchar('?');
                    else if (v == 0)
                        putchar('-');
                    else
                        putchar('0' + v);
                }
                putchar('\n');
            }
        }
	{
	    struct set *s;

	    for (s = ss->all_head; s; s = s->all_next) {
                int i;
                printf("remaining set:");
                for (i = 0; i < s->n; i++)
                    printf(" %d", s->tile[i]);
                printf(" %d\n", s->mines);
            }
	}
#endif
        if (gi->type == MINES_GRID_SQUARE && ss->n >= 2) {
            /*
             * To be compatible with older versions of Mines we need
             * to make sure the present code generates the same game.
             * Therefore, sort the remaining sets in the same order as
             * they would have had in the old code.
             */
#ifdef SOLVER_DIAGNOSTICS
            printf("Sorting the remaining sets for backwards compatibility.\n");
#endif
            struct setsort *sa = snewn(ss->n, struct setsort);
            int i = 0;
            struct set *iter;
            for (iter = ss->all_head; iter; iter = iter->all_next) {
                int j;
                int x, y;
                int xmin = iter->tile[0] % w;
                int ymin = iter->tile[0] / w;
                short mask = 0;
                mask = 1;
                for (j = 0; j < iter->n; j++) {
                    x = iter->tile[j] % w;
                    y = iter->tile[j] / w;
                    if (x < xmin) {
                        mask <<= x-xmin;
                        xmin = x;
                    }
                    mask += 1 << ((y-ymin)*3 + (x-xmin));
                }
                sa[i].this = iter;
                sa[i].tile = ymin * w + xmin;
                sa[i].mask = mask;
                i++;
            }
            qsort(sa, ss->n, sizeof(struct setsort), setsort_cmp);

            /* Now implement the new order */
            for (j = 0; j < ss->n - 1; j++)
                sa[j].this->all_next = sa[j+1].this;
            for (j = 1; j < ss->n ; j++)
                sa[j].this->all_prev = sa[j-1].this;
            sa[0].this->all_prev = NULL;
            sa[ss->n-1].this->all_next = NULL;
            ss->all_head = sa[0].this;
            ss->all_tail = sa[ss->n-1].this;

            sfree (sa);
        }

	/*
	 * Now we really are at our wits' end as far as solving
	 * this board goes. Our only remaining option is to call
	 * a perturb function and ask it to modify the board to
	 * make it easier.
	 */
	if (perturb) {
	    struct perturbations *ret;
	    struct set *s;

	    nperturbs++;

	    /*
	     * Choose a set at random from the current selection,
	     * and ask the perturb function to either fill or empty
	     * it.
	     * 
	     * If we have no sets at all, we must give up.
	     */
	    if (ss->all_head == NULL) {
#ifdef SOLVER_DIAGNOSTICS
		printf("perturbing on entire unknown set\n");
#endif
		ret = perturb(ctx, board, NULL);
	    } else {
                i = random_upto(rs, ss->n);
                for (s = ss->all_head; i > 0; i--, s = s->all_next);
#ifdef SOLVER_DIAGNOSTICS
                printf("perturbing on set ");
                setprint(s);
#endif
		ret = perturb(ctx, board, s);
	    }

	    if (ret) {
		assert(ret->n > 0);    /* otherwise should have been NULL */


		/*
		 * A number of squares have been fiddled with, and
		 * the returned structure tells us which. Adjust
		 * the mine count in any set which overlaps one of
		 * those squares, and put them back on the to-do
		 * list. Also, if the square itself is marked as a
		 * known non-mine, put it back on the squares-to-do
		 * list.
		 */
		for (i = 0; i < ret->n; i++) {
#ifdef SOLVER_DIAGNOSTICS
		    printf("perturbation %s mine at %d\n",
			   ret->changes[i].delta > 0 ? "added" : "removed",
			   ret->changes[i].tile);
#endif
		    if (ret->changes[i].delta < 0 &&
			board[ret->changes[i].tile] != -2) {
			std_add(std, ret->changes[i].tile);
		    }

		    list = ss_contains(ss, ret->changes[i].tile);

		    for (j = 0; list[j]; j++) {
			list[j]->mines += ret->changes[i].delta;
			ss_add_todo(ss, list[j]);
		    }

		    sfree(list);
		}

		/*
		 * Now free the returned data.
		 */
		sfree(ret->changes);
		sfree(ret);

#ifdef SOLVER_DIAGNOSTICS
		/*
		 * Dump the current known state of the board.
		 */
                if (gi->type == MINES_GRID_SQUARE) {
                    int x, y;
                    printf("state after perturbation:\n");
                    for (y = 0; y < h; y++) {
                        for (x = 0; x < w; x++) {
                            int v = board[y*w+x];
                            if (v == -1)
                                putchar('*');
                            else if (v == -2)
                                putchar('?');
                            else if (v == 0)
                                putchar('-');
                            else
                                putchar('0' + v);
                        }
                        putchar('\n');
                    }

                    {
                        struct set *s;
                        for (s = ss->all_head; s; s = s->all_next) {
                            printf("remaining set: ");
                            setprint(s);
                        }
                    }
                }
#endif

		/*
		 * And now we can go back round the deductive loop.
		 */
		continue;
	    }
	}

	/*
	 * If we get here, even that didn't work (either we didn't
	 * have a perturb function or it returned failure), so we
	 * give up entirely.
	 */
	break;
    }

    /*
     * See if we've got any unknown squares left.
     */
    for (i = 0; i < gi->ntiles; i++)
        if (board[i] == -2) {
            nperturbs = -1;	       /* failed to complete */
            break;
        }

    /*
     * Free the set list and square-todo list.
     */
    {
	struct set *s;
        if (ss->all_head) {
            for (s = ss->all_head->all_next; s; s = s->all_next)
                sfree(s->all_prev);
            sfree(ss->all_tail);
        }
	sfree(ss);
	sfree(std->next);
    }

    return nperturbs;
}

/* ----------------------------------------------------------------------
 * Grid generator which uses the above solver.
 */

struct minectx {
    bool *mines, *opened;
    int w, h;
    struct grid_info *grid;
    int start_tile;
    bool allow_big_perturbs;
    int nperturbs_since_last_new_open;
    random_state *rs;
};

static int mineopen(void *vctx, int tile)
{
    struct minectx *ctx = (struct minectx *)vctx;
    int i, n;

    assert(0 <= tile && tile < ctx->grid->ntiles);
    if (ctx->mines[tile])
	return -1;		       /* *bang* */

    if (!ctx->opened[tile]) {
        ctx->opened[tile] = true;
        ctx->nperturbs_since_last_new_open = 0;
    }

    n = 0;
    for (i = tile*ctx->grid->nsize; i < (tile+1)*ctx->grid->nsize; i++)
        if (ctx->grid->neighbours[i] >= 0 &&
            ctx->mines[ctx->grid->neighbours[i]])
            n++;

    return n;
}

/* Structure used internally to mineperturb(). */
struct square {
    int tile, type, random;
};

static int squarecmp(const void *av, const void *bv)
{
    const struct square *a = (const struct square *)av;
    const struct square *b = (const struct square *)bv;
    if (a->type < b->type)
	return -1;
    else if (a->type > b->type)
	return +1;
    else if (a->random < b->random)
	return -1;
    else if (a->random > b->random)
	return +1;
    else if (a->tile < b->tile)
	return -1;
    else if (a->tile > b->tile)
	return +1;
    return 0;
}

/*
 * Normally this function is passed a set description.
 * On occasions, though, there is no _localised_ set being used,
 * and the set being perturbed is supposed to be the entirety of
 * the unreachable area. This is signified by the special case
 * s == NULL: in this case, anything labelled -2 in the board is part
 * of the set.
 * 
 * Allowing perturbation in this special case appears to make it
 * guaranteeably possible to generate a workable board for any mine
 * density, but they tend to be a bit boring, with mines packed
 * densely into far corners of the board and the remainder being
 * less dense than one might like. Therefore, to improve overall
 * board quality I disable this feature for the first few attempts,
 * and fall back to it after no useful board has been generated.
 */
static struct perturbations *mineperturb(void *vctx, signed char *board,
					 struct set *s)
{
    struct minectx *ctx = (struct minectx *)vctx;
    struct square *sqlist;
    int i, tile, n, nfull, nempty;
    struct square **tofill, **toempty, **todo;
    int ntofill, ntoempty, ntodo, dtodo, dset;
    struct perturbations *ret;
    int *setlist;

    if (s == NULL && !ctx->allow_big_perturbs) {
#ifdef GENERATION_DIAGNOSTICS
	printf("big perturbs forbidden on this run\n");
#endif
	return NULL;
    }

    ctx->nperturbs_since_last_new_open++;
    if (ctx->nperturbs_since_last_new_open >= ctx->w ||
        ctx->nperturbs_since_last_new_open >= ctx->h) {
#ifdef GENERATION_DIAGNOSTICS
	printf("too many perturb attempts without opening a new square\n");
#endif
	return NULL;
    }

#ifdef GENERATION_DIAGNOSTICS
    if (ctx->grid->type == MINES_GRID_SQUARE ||
        ctx->grid->type == MINES_GRID_SQUARE_CYCLIC) {
	int yy, xx;
	printf("board before perturbing:\n");
	for (yy = 0; yy < ctx->h; yy++) {
	    for (xx = 0; xx < ctx->w; xx++) {
                int tile = yy*ctx->w+xx;
		int v = ctx->mines[tile];
		if (tile == ctx->start_tile) {
		    assert(!v);
		    putchar('S');
		} else if (v) {
		    putchar('*');
		} else {
		    putchar('-');
		}
	    }
	    putchar('\n');
	}
	printf("\n");
    }
#endif

    /*
     * Make a list of all the squares in the board which we can
     * possibly use. This list should be in preference order, which
     * means
     * 
     *  - first, unknown squares on the boundary of known space
     *  - next, unknown squares beyond that boundary
     * 	- as a very last resort, known squares, but not within one
     * 	  square of the starting position.
     * 
     * Each of these sections needs to be shuffled independently.
     * We do this by preparing list of all squares and then sorting
     * it with a random secondary key.
     */
    sqlist = snewn(ctx->grid->ntiles, struct square);
    n = 0;
    for (tile = 0; tile < ctx->grid->ntiles; tile++) {
        /*
         * If this square is too near the starting position,
         * don't put it on the list at all.
         */
        if (tile == ctx->start_tile ||
            is_neighbour_of(ctx->grid, tile, ctx->start_tile))
            continue;

        /*
         * If this square is in the input set, also don't put
         * it on the list!
         */
        if ((s == NULL && board[tile] == -2) ||
            ( s != NULL && setcontains(s, tile)))
            continue;

        sqlist[n].tile = tile;

        if (board[tile] != -2) {
            sqlist[n].type = 3;    /* known square */
        } else {
            /*
             * Unknown square. Examine everything around it and
             * see if it borders on any known squares. If it
             * does, it's class 1, otherwise it's 2.
             */
            int j, ni;
            sqlist[n].type = 2;

            for (j = 0; j < ctx->grid->nsize; j++) {
                ni = ctx->grid->neighbours[tile*ctx->grid->nsize + j];
                if (ni >= 0 && board[ni] != -2) {
                    sqlist[n].type = 1;
                    break;
                }
            }
        }

        /*
         * Finally, a random number to cause qsort to
         * shuffle within each group.
         */
        sqlist[n].random = random_bits(ctx->rs, 31);

        n++;
    }

    qsort(sqlist, n, sizeof(struct square), squarecmp);

    /*
     * Now count up the number of full and empty squares in the set
     * we've been provided.
     */
#ifdef GENERATION_DIAGNOSTICS
    printf("perturb wants to fill or empty these squares:");
#endif
    nfull = nempty = 0;
    if (s) {
        for (i = 0; i < s->n; i++) {
            tile = s->tile[i];
#ifdef GENERATION_DIAGNOSTICS
            printf(" (%d)", tile);
#endif
            if (ctx->mines[tile])
                nfull++;
            else
                nempty++;
        }
    } else {
        for (i = 0; i < ctx->grid->ntiles; i++) {
            if (board[i] == -2) {
#ifdef GENERATION_DIAGNOSTICS
                printf(" (%d)", i);
#endif
                if (ctx->mines[i])
                    nfull++;
                else
                    nempty++;
            }
        }
    }

#ifdef GENERATION_DIAGNOSTICS
    {
        int i;
	printf("\nperturb set includes %d full, %d empty\n", nfull, nempty);
        printf("source squares in preference order:");
        for (i = 0; i < n; i++)
            printf(" %d", sqlist[i].tile);
        printf("\n");
    }
#endif

    /*
     * Now go through our sorted list until we find either `nfull'
     * empty squares, or `nempty' full squares; these will be
     * swapped with the appropriate squares in the set to either
     * fill or empty the set while keeping the same number of mines
     * overall.
     */
    ntofill = ntoempty = 0;
    if (s) {
	tofill = snewn(n, struct square *);
	toempty = snewn(n, struct square *);
    } else {
	tofill = snewn(ctx->grid->ntiles, struct square *);
	toempty = snewn(ctx->grid->ntiles, struct square *);
    }
    for (i = 0; i < n; i++) {
	struct square *sq = &sqlist[i];
	if (ctx->mines[sq->tile])
	    toempty[ntoempty++] = sq;
	else
	    tofill[ntofill++] = sq;
	if (ntofill == nfull || ntoempty == nempty)
	    break;
    }

#ifdef GENERATION_DIAGNOSTICS
    printf("can fill %d (of %d) or empty %d (of %d)\n",
           ntofill, nfull, ntoempty, nempty);
#endif

    /*
     * If we haven't found enough empty squares outside the set to
     * empty it into _or_ enough full squares outside it to fill it
     * up with, we'll have to settle for doing only a partial job.
     * In this case we choose to always _fill_ the set (because
     * this case will tend to crop up when we're working with very
     * high mine densities and the only way to get a solvable board
     * is going to be to pack most of the mines solidly around the
     * edges). So now our job is to make a list of the empty
     * squares in the set, and shuffle that list so that we fill a
     * random selection of them.
     */
    if (ntofill != nfull && ntoempty != nempty) {
	int k;

	assert(ntoempty != 0);

	setlist = snewn(ctx->grid->ntiles, int);
	i = 0;
	if (s) {
            for (i = 0; i < s->n; i++) {
                tile = s->tile[i];
                if (!ctx->mines[tile])
                    setlist[i++] = tile;
            }
	} else {
            int tile;
            for (tile = 0; tile < ctx->grid->ntiles; tile++)
                if (board[tile] == -2) {
                    if (!ctx->mines[tile])
                        setlist[i++] = tile;
                }
	}
	assert(i > ntoempty);
	/*
	 * Now pick `ntoempty' items at random from the list.
	 */
#ifdef GENERATION_DIAGNOSTICS
        printf("doing a partial fill:");
#endif

	for (k = 0; k < ntoempty; k++) {
	    int index = k + random_upto(ctx->rs, i - k);
	    int tmp;

	    tmp = setlist[k];
	    setlist[k] = setlist[index];
	    setlist[index] = tmp;

#ifdef GENERATION_DIAGNOSTICS
            printf(" (%d)", setlist[index]);
#endif
	}
#ifdef GENERATION_DIAGNOSTICS
        printf("\n");
#endif
    } else
	setlist = NULL;

    /*
     * Now we're pretty much there. We need to either
     * 	(a) put a mine in each of the empty squares in the set, and
     * 	    take one out of each square in `toempty'
     * 	(b) take a mine out of each of the full squares in the set,
     * 	    and put one in each square in `tofill'
     * depending on which one we've found enough squares to do.
     * 
     * So we start by constructing our list of changes to return to
     * the solver, so that it can update its data structures
     * efficiently rather than having to rescan the whole board.
     */
    ret = snew(struct perturbations);
    if (ntofill == nfull) {
	todo = tofill;
	ntodo = ntofill;
	dtodo = +1;
	dset = -1;
	sfree(toempty);
    } else {
	/*
	 * (We also fall into this case if we've constructed a
	 * setlist.)
	 */
	todo = toempty;
	ntodo = ntoempty;
	dtodo = -1;
	dset = +1;
	sfree(tofill);
    }
    ret->n = 2 * ntodo;
    ret->changes = snewn(ret->n, struct perturbation);
    for (i = 0; i < ntodo; i++) {
	ret->changes[i].tile = todo[i]->tile;
	ret->changes[i].delta = dtodo;
    }
    /* now i == ntodo */
    if (setlist) {
	int j;
	assert(todo == toempty);
	for (j = 0; j < ntoempty; j++) {
	    ret->changes[i].tile = setlist[j];
	    ret->changes[i].delta = dset;
	    i++;
	}
	sfree(setlist);
    } else if (s) {
        int j;
        for (j = 0; j < s->n; j++) {
            tile = s->tile[j];
            int currval = (ctx->mines[tile] ? +1 : -1);
            if (dset == -currval) {
                ret->changes[i].tile = tile;
                ret->changes[i].delta = dset;
                i++;
            }
        }
    } else {
        int tile;
        for (tile = 0; tile < ctx->grid->ntiles; tile++) {
            if (board[tile] == -2) {
                int currval = (ctx->mines[tile] ? +1 : -1);
                if (dset == -currval) {
                    ret->changes[i].tile = tile;
                    ret->changes[i].delta = dset;
                    i++;
                }
            }
        }
    }
    assert(i == ret->n);

    sfree(sqlist);
    sfree(todo);

    /*
     * Having set up the precise list of changes we're going to
     * make, we now simply make them and return.
     */
    for (i = 0; i < ret->n; i++) {
	int delta, tile;

	tile = ret->changes[i].tile;
	delta = ret->changes[i].delta;

	/*
	 * Check we're not trying to add an existing mine or remove
	 * an absent one.
	 */
	assert((delta < 0) ^ (ctx->mines[tile] == 0));

	/*
	 * Actually make the change.
	 */
	ctx->mines[tile] = (delta > 0);

	/*
	 * Update any numbers already present in the board.
	 */
        int j;
        for (j = 0; j < ctx->grid->nsize; j++) {
            int ni = ctx->grid->neighbours[tile*ctx->grid->nsize+j];
            if (ni >= 0 && board[ni] != -2) {
                if (board[ni] >= 0)
                    board[ni] += delta;
            }
        }
        if (board[tile] != -2) {
            /*
             * The square itself is marked as known in
             * the board. Mark it as a mine if it's a
             * mine, or else work out its number.
             */
            if (delta > 0) {
                board[tile] = -1;
            } else {
                int j2, ni2, minecount = 0;
                for (j2 = 0; j2 < ctx->grid->nsize; j2++) {
                    ni2 = ctx->grid->neighbours[tile*ctx->grid->nsize+j2];
                    if (ni2 >= 0 && ctx->mines[ni2])
                        minecount++;
                }
                board[tile] = minecount;
            }
        }
    }

#ifdef GENERATION_DIAGNOSTICS
    if (ctx->grid->type == MINES_GRID_SQUARE ||
        ctx->grid->type == MINES_GRID_SQUARE_CYCLIC) {
	int yy, xx;
	printf("board after perturbing:\n");
	for (yy = 0; yy < ctx->h; yy++) {
	    for (xx = 0; xx < ctx->w; xx++) {
                int tile = yy*ctx->w+xx;
		int v = ctx->mines[tile];
		if (tile == ctx->start_tile) {
		    assert(!v);
		    putchar('S');
		} else if (v) {
		    putchar('*');
		} else {
		    putchar('-');
		}
	    }
	    putchar('\n');
	}
	printf("\n");
    }
#endif

    return ret;
}

static bool *minegen(struct grid_info *gi, int w, int h, int n_orig, int tile,
                     bool unique, random_state *rs)
{
    bool *ret = snewn(gi->ntiles, bool);
    bool success;
    int ntries = 0;

    do {
        int n;

	success = false;
	ntries++;

	memset(ret, 0, gi->ntiles);

	/*
	 * Start by placing n mines (or as many as we can), none of
	 * which is at x,y or within one square of it.
	 */
	{
	    int *tmp = snewn(gi->ntiles, int);
	    int i, k, nn;

	    /*
	     * Write down the list of possible mine locations.
	     */
	    k = 0;
	    for (i = 0; i < gi->ntiles; i++)
                if (i != tile && !is_neighbour_of(gi, i, tile))
                    tmp[k++] = i;

	    /*
	     * Now pick n off the list at random. If we run out of
	     * places to put mines, reduce nn to the maximum number we
	     * _can_ place.
	     */
	    n = nn = (n_orig < k ? n_orig : k);
	    while (nn-- > 0) {
		i = random_upto(rs, k);
		ret[tmp[i]] = true;
		tmp[i] = tmp[--k];
	    }

	    sfree(tmp);
	}

#ifdef GENERATION_DIAGNOSTICS
        if (gi->type == MINES_GRID_SQUARE ||
            gi->type == MINES_GRID_SQUARE_CYCLIC) {
	    int yy, xx;
	    printf("board after initial generation:\n");
	    for (yy = 0; yy < h; yy++) {
		for (xx = 0; xx < w; xx++) {
                    int dind = yy*w+xx;
		    int v = ret[dind];
		    if (dind == tile) {
			assert(!v);
			putchar('S');
		    } else if (v) {
			putchar('*');
		    } else {
			putchar('-');
		    }
		}
		putchar('\n');
	    }
	    printf("\n");
	}
#endif

	/*
	 * Now set up a results board to run the solver in, and a
	 * context for the solver to open squares. Then run the solver
	 * repeatedly; if the number of perturb steps ever goes up or
	 * it ever returns -1, give up completely.
	 *
	 * We bypass this bit if we're not after a unique board.
         */
	if (unique) {
	    signed char *solvegrid = snewn(gi->ntiles, signed char);
            bool *opened = snewn(gi->ntiles, bool);
	    struct minectx actx, *ctx = &actx;
	    int solveret, prevret = -2;

            memset(opened, 0, gi->ntiles * sizeof(bool));

	    ctx->mines = ret;
            ctx->opened = opened;
	    ctx->w = w;
	    ctx->h = h;
            ctx->grid = gi;
            gi->refcount++;
	    ctx->start_tile = tile;
	    ctx->rs = rs;
	    ctx->allow_big_perturbs = (ntries > 100);
            ctx->nperturbs_since_last_new_open = 0;

	    while (1) {
		memset(solvegrid, -2, gi->ntiles);
		solvegrid[tile] = mineopen(ctx, tile);
		assert(solvegrid[tile] == 0); /* by deliberate arrangement */

		solveret =
		    minesolve(gi, w, h, n, solvegrid, mineopen, mineperturb, ctx, rs);
		if (solveret < 0 || (prevret >= 0 && solveret >= prevret)) {
		    success = false;
		    break;
		} else if (solveret == 0) {
		    success = true;
		    break;
		}
	    }

            gi->refcount--;
            assert(gi->refcount > 0);
	    sfree(solvegrid);
	    sfree(opened);
	} else {
	    success = true;
	}

    } while (!success);

    return ret;
}

static char *describe_layout(enum game_type type, bool *mines, int area,
                             int w, int tile, int n, bool obfuscate,
                             char *desc)
{
    char *ret, *p;
    unsigned char *bmp;
    int i, desclen;

    /*
     * Set up the mine bitmap and obfuscate it.
     */
    bmp = snewn((area + 7) / 8, unsigned char);
    memset(bmp, 0, (area + 7) / 8);
    for (i = 0; i < area; i++) {
        if (mines[i]) {
            bmp[i / 8] |= 0x80 >> (i % 8);
            n--;
        }
    }
    if (obfuscate)
        obfuscate_bitmap(bmp, area, false);

    /*
     * Now encode the resulting bitmap in hex. We can work to
     * nibble rather than byte granularity, since the obfuscation
     * function guarantees to return a bit string of the same
     * length as its input.
     */
    if (desc)
        desclen = strlen(desc) + 1;
    else
        desclen = 0;
    ret = snewn((area+3)/4 + 100 + desclen, char);
    if (type == MINES_GRID_SQUARE) {
        int x = tile % w;
        int y = tile / w;
        p = ret + sprintf(ret, "%d,%d,%s", x, y,
                          obfuscate ? "m" : "u");   /* 'm' == masked */
    } else if (desc) {
        p = ret + sprintf(ret, "%s_%d,%d,%s", desc, tile, area,
                          obfuscate ? "m" : "u");   /* 'm' == masked */
    } else {
        p = ret + sprintf(ret, "%d,%d,%s", tile, area,
                          obfuscate ? "m" : "u");   /* 'm' == masked */
    }
    for (i = 0; i < (area+3)/4; i++) {
        int v = bmp[i/2];
        if (i % 2 == 0)
            v >>= 4;
        *p++ = "0123456789abcdef"[v & 0xF];
    }
    /* Encode an indication that not all mines were placed */
    if (n > 0)
        sprintf(p, "+%d", n);
    else
        *p = '\0';

    sfree(bmp);

    return ret;
}

static bool *new_mine_layout(struct grid_info *gi, int w, int h, int n,
                             int tile, bool unique,
			     random_state *rs, char **game_desc)
{
#ifdef TIME_MINELAYOUT
    clock_t before, after;
    before = clock();
#endif
    bool *mines = minegen(gi, w, h, n, tile, unique, rs);
#ifdef TIME_MINELAYOUT
    after = clock();
    printf("Mine layout time: %ld ms\n", 1000*(after-before)/CLOCKS_PER_SEC);
#endif

    if (game_desc)
        *game_desc = describe_layout(gi->type, mines, gi->ntiles, w, tile, n,
                                     true, gi->desc);

    return mines;
}

/* ----------------------------------------------------------------------
 * game description
 */

#define GRID_DESC_SEP '_'

static char *extract_grid_desc(const char **desc)
{
    char *sep = strchr(*desc, GRID_DESC_SEP), *gd;
    int gd_len;

    if (!sep) return NULL;

    gd_len = sep - (*desc);
    gd = snewn(gd_len+1, char);
    memcpy(gd, *desc, gd_len);
    gd[gd_len] = '\0';

    *desc = sep+1;

    return gd;
}

static char *new_game_desc(const game_params *params, random_state *rs,
			   char **aux, bool interactive)
{
    int tile;
    char *grid_desc;
    struct grid_info *gi = NULL;

    /*
     * Handle grids requiring a nontrivial grid_desc. We don't always
     * want to make the default choice that grid.c would make.
     */
    switch (params->type) {
      case MINES_GRID_TRIANGULAR:
        /*
         * For the non-cyclic triangular grid we use the newer
         * 'version 0' grid type which avoids having two 'ears' on the
         * left edge of the grid. However for
         * MINES_GRID_TRIANGULAR_CYCLIC we deliberately don't do this,
         * and use a NULL grid_desc, which puts those ears back in, so
         * that the two sides of the grid will link up correctly.
         */
        grid_desc = snewn(2, char);
        grid_desc[0] = '0';
        grid_desc[1] = '\0';
        break;
      case MINES_GRID_PENROSE_P2:
      case MINES_GRID_PENROSE_P3:
      case MINES_GRID_HATS:
      case MINES_GRID_SPECTRES:
        grid_desc = grid_new_desc(grid_types[params->type],
                                  params->w, params->h, rs);
        break;
      default:
        /*
         * We make this the default instead of calling grid_new_desc.
         * The rationale is to be future safe.  If any development is
         * done in the grid generation code, then we don't want to
         * include that without checking or updating a lot of limits,
         * such as max neighbours etc.
         */
        grid_desc = NULL;
    }

    /*
     * We generate the coordinates of an initial click even if they
     * aren't actually used. This has the effect of harmonising the
     * random number usage between interactive and batch use: if
     * you use `mines --generate' with an explicit random seed, you
     * should get exactly the same results as if you type the same
     * random seed into the interactive game and click in the same
     * initial location. (Of course you won't get the same board if
     * you click in a _different_ initial location, but there's
     * nothing to be done about that.)
     */

    if (params->type == MINES_GRID_SQUARE) {
        /* Consume two random numbers for the Square grid for
         * backwards compatibility with older versions.
         */
        int x = random_upto(rs, params->w);
        int y = random_upto(rs, params->h);
        tile = y*params->w+x;
    } else {
        gi = new_grid(params, grid_desc);
        tile = random_upto(rs, gi->ntiles);
    }

    /*
     * Override with params->first_click_tile if it is set. (For
     * the same reason, we still generated the random numbers first.)
     */
    if (params->first_click_tile >= 0) {
        tile = params->first_click_tile;

        /*
         * If params->first_click_tile was too large, either by simple
         * accident or variable grid size, coerce it to fit. That way
         * we don't have to predict the right limit in advance in
         * validate_params.
         */
        if (!gi)
            gi = new_grid(params, grid_desc);
        if (tile >= gi->ntiles)
            tile = gi->ntiles - 1;
    }

    if (!interactive) {
	/*
	 * For batch-generated boards, pre-open one square.
	 */
	bool *grid;
	char *game_desc;

        if (!gi)
            gi = new_grid(params, grid_desc);

	grid = new_mine_layout(gi, params->w, params->h, params->n,
			       tile, params->unique, rs, &game_desc);

        free_grid_info(gi);  /* we don't bother with refcount here. */
	sfree(grid);
        sfree(grid_desc);
        return game_desc;
    } else {
	char *rsdesc, *desc;
        int len;
	rsdesc = random_state_encode(rs);
        if (grid_desc)
            len = strlen(grid_desc) + 1 + strlen(rsdesc) + 100;
        else
            len = strlen(rsdesc) + 100;
	desc = snewn(len, char);
        if (grid_desc) {
            sprintf(desc, "%s%cr%d,%c,%s", grid_desc, GRID_DESC_SEP,
                    params->n, (char)(params->unique ? 'u' : 'a'), rsdesc);
            sfree(grid_desc);
        } else {
            sprintf(desc, "r%d,%c,%s", params->n, (char)(params->unique ? 'u' : 'a'), rsdesc);
        }
	sfree(rsdesc);
        if (gi)
            free_grid_info(gi);  /* we don't bother with refcount here. */
	return desc;
    }
}

static const char *validate_desc(const game_params *params, const char *desc)
{
    int x, y, tile, ntiles;
    char *grid_desc;
    const char *grid_error;

    /*
     * Pull a grid_desc off the front of the string, if any, and check
     * if it's correct.
     */
    grid_desc = extract_grid_desc(&desc);
    grid_error = grid_validate_desc(grid_types[params->type],
                                    params->w, params->h, grid_desc);
    if (grid_error) {
        sfree(grid_desc);
        return grid_error;
    }
    switch (params->type) {
      case MINES_GRID_TRIANGULAR:
      case MINES_GRID_PENROSE_P2:
      case MINES_GRID_PENROSE_P3:
      case MINES_GRID_HATS:
      case MINES_GRID_SPECTRES:
        if (!grid_desc)
            return "This gridtype requires a grid description";
        break;
      default:
        /*
         * Not only do we want to catch MINES_GRID_TRIANGULAR_CYCLIC
         * here but also other grids that grid_validate_desc in the
         * future will think is ok.
         */
        if (grid_desc) {
            sfree(grid_desc);
            return "This gridtype may not have a grid description";
        }
    }
    /* Generate the grid, to find out how many tiles it has. */
    {
        grid *grid;
        grid = grid_new(grid_types[params->type],
                        params->w, params->h, grid_desc);
        ntiles = grid->num_faces;
        grid_free(grid);
    }
    sfree(grid_desc);
    if (*desc == 'r') {
        desc++;
	if (!*desc || !isdigit((unsigned char)*desc))
	    return "No initial mine count in game description";
	while (*desc && isdigit((unsigned char)*desc))
	    desc++;		       /* skip over mine count */
	if (*desc != ',')
	    return "No ',' after initial x-coordinate in game description";
	desc++;
	if (*desc != 'u' && *desc != 'a')
	    return "No uniqueness specifier in game description";
	desc++;
	if (*desc != ',')
	    return "No ',' after uniqueness specifier in game description";
	/* now ignore the rest */
    } else {
        size_t hexlen;

	if (*desc && isdigit((unsigned char)*desc)) {
            if (params->type == MINES_GRID_SQUARE) {
                x = atoi(desc);
                if (x < 0 || x >= params->w)
                    return "Initial x-coordinate was out of range";
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over x coordinate */
                if (*desc != ',')
                    return "No ',' after initial x-coordinate in game description";
                desc++;		       /* eat comma */
                if (!*desc || !isdigit((unsigned char)*desc))
                    return "No initial y-coordinate in game description";
                y = atoi(desc);
                if (y < 0 || y >= params->h)
                    return "Initial y-coordinate was out of range";
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over y coordinate */
                if (*desc != ',')
                    return "No ',' after initial y-coordinate in game description";
                desc++;		       /* eat comma */
            } else {
                tile = atoi(desc);
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over tile */
                if (*desc != ',')
                    return "No ',' after initial tile-coordinate in game description";
                desc++;		       /* eat comma */
                int ntiles_desc = atoi(desc);
                if (ntiles > 0 && ntiles != ntiles_desc)
                    return "Number of tiles differ from actual number of tiles in grid";
                ntiles = ntiles_desc;
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over number of tiles */
                if (*desc != ',')
                    return "No ',' after initial numer of tiles in game description";
                desc++;		       /* eat comma */
                if (tile < 0 || tile >= ntiles)
                    return "Initial tile-coordinate was out of range";
            }
	}
	/* eat `m' for `masked' or `u' for `unmasked', if present */
	if (*desc == 'm' || *desc == 'u')
	    desc++;
        /* expect a hex string of the right length */
        hexlen = strspn(desc, "0123456789abcdefABCDEF");
	if (hexlen != (ntiles+3)/4)
	    return "Description of mine layout is wrong length";
        desc += hexlen;
        /* accept a +nnn at the end indicating unplaced mines */
        if (*desc == '+') {
            desc++;
            int unplaced = atoi(desc);
            desc += strspn(desc, "0123456789");
            if (unplaced <= 0)
                return "Invalid unplaced-mines count";
        }
        if (*desc)
            return "Trailing junk in game description";
    }

    return NULL;
}

/* ----------------------------------------------------------------------
 * Game routines
 */

static int open_square(game_state *state, int tile)
{
    int w = state->w, h = state->h;
    int ii, nmines, ncovered;

    if (!state->layout->mines) {
	/*
	 * We have a preliminary game in which the mine layout
	 * hasn't been generated yet. Generate it based on the
	 * initial click location.
	 */
	char *desc, *privdesc, *tmp = NULL;
	state->layout->mines = new_mine_layout(state->grid, w, h, state->layout->n,
					       tile, state->layout->unique,
					       state->layout->rs,
					       &desc);

        /* Record the first-click location, so that if the user
         * undoes this move they can still remember where it was. */
        state->layout->start_tile = tile;

	/*
	 * Find the trailing substring of the game description
	 * corresponding to just the mine layout; we will use this
	 * as our second `private' game ID for serialisation.
	 */
        privdesc = strchr(desc, GRID_DESC_SEP);
        if (privdesc)
            privdesc++;
        else
            privdesc = desc;
	while (*privdesc && isdigit((unsigned char)*privdesc)) privdesc++;
	if (*privdesc == ',') privdesc++;
	while (*privdesc && isdigit((unsigned char)*privdesc)) privdesc++;
	if (*privdesc == ',') privdesc++;
	assert(*privdesc == 'm');
        if (state->grid->desc) {
            tmp = privdesc;
            privdesc = snewn(strlen(state->grid->desc) + 1 + strlen(tmp) + 1, char);
            sprintf(privdesc, "%s%c%s", state->grid->desc, GRID_DESC_SEP, tmp);
        }
	midend_supersede_game_desc(state->layout->me, desc, privdesc);
        if (tmp)
            sfree(privdesc);
	random_free(state->layout->rs);
	state->layout->rs = NULL;
    }

    if (state->layout->mines[tile]) {
	/*
	 * The player has landed on a mine. Bad luck. Expose the
	 * mine that killed them, but not the rest (in case they
	 * want to Undo and carry on playing).
	 */
	state->dead = true;
	state->board[tile] = 65;
	return -1;
    }

    /*
     * Otherwise, the player has opened a safe square. Mark it to-do.
     */
    state->board[tile] = -10;	       /* `todo' value internal to this func */

    /*
     * Now go through the board finding all `todo' values and
     * opening them. Every time one of them turns out to have no
     * neighbouring mines, we add all its unopened neighbours to
     * the list as well.
     * 
     * FIXME: We really ought to be able to do this better than
     * using repeated N^2 scans of the board.
     */
    while (1) {
	bool done_something = false;
        for (ii = 0; ii < state->grid->ntiles; ii++)
            if (state->board[ii] == -10) {
                int j, ni, v;
                int nsize = state->grid->nsize;

                assert(!state->layout->mines[ii]);

                v = 0;

                for (j = 0; j < state->grid->nsize; j++) {
                    ni = state->grid->neighbours[ii*nsize + j];
                    if (ni >= 0 && state->layout->mines[ni])
                        v++;
                }

                state->board[ii] = v;

                if (v == 0) {
                    for (j = 0; j < state->grid->nsize; j++) {
                        ni = state->grid->neighbours[ii*nsize + j];
                        if (ni >= 0 &&
                            state->board[ni] == -2)
                            state->board[ni] = -10;
                    }
                }

                done_something = true;
            }

	if (!done_something)
	    break;
    }

    /* If the player has already lost, don't let them win as well. */
    if (state->dead) return 0;
    /*
     * Finally, scan the grid and see if exactly as many squares
     * are still covered as there are mines. If so, set the `won'
     * flag and fill in mine markers on all covered squares.
     */
    nmines = ncovered = 0;
    for (ii = 0; ii < state->grid->ntiles; ii++) {
        if (state->board[ii] < 0)
            ncovered++;
        if (state->layout->mines[ii])
            nmines++;
    }
    assert(ncovered >= nmines);
    if (ncovered == nmines) {
        for (ii = 0; ii < state->grid->ntiles; ii++) {
            if (state->board[ii] < 0)
                state->board[ii] = -1;
	}
	state->won = true;
    }

    return 0;
}

static game_state *new_game(midend *me, const game_params *params,
                            const char *desc)
{
    game_state *state = snew(game_state);
    int i, wh, x, y, tile;
    bool masked;
    unsigned char *bmp;
    char *grid_desc;

    state->w = params->w;
    state->h = params->h;
    state->n = params->n;
    state->dead = state->won = false;
    state->used_solve = false;

    grid_desc = extract_grid_desc(&desc);
    state->grid = new_grid(params, grid_desc);
    sfree(grid_desc);

    wh = state->grid->ntiles;

    state->layout = snew(struct mine_layout);
    memset(state->layout, 0, sizeof(struct mine_layout));
    state->layout->refcount = 1;
    state->layout->start_tile = -1;

    state->board = snewn(wh, signed char);
    memset(state->board, -2, wh);

    if (*desc == 'r') {
	desc++;
	state->layout->n = atoi(desc);
	while (*desc && isdigit((unsigned char)*desc))
	    desc++;		       /* skip over mine count */
	if (*desc) desc++;	       /* eat comma */
	if (*desc == 'a')
	    state->layout->unique = false;
	else
	    state->layout->unique = true;
	desc++;
	if (*desc) desc++;	       /* eat comma */

	state->layout->mines = NULL;
	state->layout->rs = random_state_decode(desc);
	state->layout->me = me;

    } else {
	state->layout->rs = NULL;
	state->layout->me = NULL;
	state->layout->mines = snewn(wh, bool);

        if (params->type == MINES_GRID_SQUARE) {
            if (*desc && isdigit((unsigned char)*desc)) {
                x = atoi(desc);
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over x coordinate */
                if (*desc) desc++;	       /* eat comma */
                y = atoi(desc);
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over y coordinate */
                if (*desc) desc++;	       /* eat comma */
                tile = y*params->w + x;
            } else {
                tile = -1;
            }
        } else {
            if (*desc && isdigit((unsigned char)*desc)) {
                tile = atoi(desc);
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over tile coordinate */
                if (*desc) desc++;	       /* eat comma */
            } else {
                tile = -1;
            }
            if (*desc && isdigit((unsigned char)*desc)) {
                /* Ignore ntiles.  We alredy have that information and
                 * any error should have been caught by validate_desc.
                 */
                while (*desc && isdigit((unsigned char)*desc))
                    desc++;		       /* skip over number of tiles */
                if (*desc) desc++;	       /* eat comma */

            }
        }

	if (*desc == 'm') {
	    masked = true;
	    desc++;
	} else {
	    if (*desc == 'u')
		desc++;
	    /*
	     * We permit game IDs to be entered by hand without the
	     * masking transformation.
	     */
	    masked = false;
	}

	bmp = snewn((wh + 7) / 8, unsigned char);
	memset(bmp, 0, (wh + 7) / 8);
	for (i = 0; i < (wh+3)/4; i++) {
	    int c = *desc++;
	    int v;

	    assert(c != 0);	       /* validate_desc should have caught */
	    if (c >= '0' && c <= '9')
		v = c - '0';
	    else if (c >= 'a' && c <= 'f')
		v = c - 'a' + 10;
	    else if (c >= 'A' && c <= 'F')
		v = c - 'A' + 10;
	    else
		v = 0;

	    bmp[i / 2] |= v << (4 * (1 - (i % 2)));
	}

	if (masked)
	    obfuscate_bitmap(bmp, wh, true);

        state->layout->n = 0;
	memset(state->layout->mines, 0, wh * sizeof(bool));
	for (i = 0; i < wh; i++) {
	    if (bmp[i / 8] & (0x80 >> (i % 8))) {
		state->layout->mines[i] = true;
		state->layout->n++;
            }
	}

        if (*desc == '+') {
            /* The game description indicates some mines were unplaced */
            int unplaced = atoi(desc);
            state->layout->n += unplaced;
        }

	if (tile >= 0)
	    open_square(state, tile);
        sfree(bmp);
    }

    return state;
}

static void set_public_desc(game_state *state, const char *pubdesc)
{
    int start_tile = -1;

    /* Skip grid description if present */
    extract_grid_desc(&pubdesc);

    if (state->grid->type == MINES_GRID_SQUARE) {
        int x = -1, y = -1;

        if (*pubdesc && isdigit((unsigned char)*pubdesc)) {
            x = atoi(pubdesc);
            while (*pubdesc && isdigit((unsigned char)*pubdesc))
                pubdesc++;                 /* skip over x coordinate */
            if (*pubdesc) pubdesc++;       /* eat comma */
            y = atoi(pubdesc);
        }

        if (x >= 0 && y >= 0 && x < state->w && y < state->h) {
            start_tile = y*state->w + x;
        }
    } else {
        if (*pubdesc && isdigit((unsigned char)*pubdesc)) {
            start_tile = atoi(pubdesc);
            /* Ignore ntiles, we already have that information. */
        }
    }
    state->layout->start_tile = start_tile;
}

static game_state *dup_game(const game_state *state)
{
    game_state *ret = snew(game_state);

    ret->w = state->w;
    ret->h = state->h;
    ret->n = state->n;
    ret->dead = state->dead;
    ret->won = state->won;
    ret->used_solve = state->used_solve;
    ret->layout = state->layout;
    ret->layout->refcount++;
    ret->grid = state->grid;
    ret->grid->refcount++;
    ret->board = snewn(state->grid->ntiles, signed char);
    memcpy(ret->board, state->board, state->grid->ntiles);
    return ret;
}

static void free_game(game_state *state)
{
    if (--state->layout->refcount <= 0) {
	sfree(state->layout->mines);
	if (state->layout->rs)
	    random_free(state->layout->rs);
	sfree(state->layout);
    }
    if (--state->grid->refcount <= 0)
	free_grid_info(state->grid);
    sfree(state->board);
    sfree(state);
}

static char *solve_game(const game_state *state, const game_state *currstate,
                        const char *aux, const char **error)
{
    if (!state->layout->mines) {
	*error = "Game has not been started yet";
	return NULL;
    }

    return dupstr("S");
}

static bool game_can_format_as_text_now(const game_params *params)
{
    return params->type == MINES_GRID_SQUARE ||
        params->type == MINES_GRID_SQUARE_CYCLIC ||
        params->type == MINES_GRID_OCTAGONAL2;
}

static char *game_text_format(const game_state *state)
{
    char *ret;
    int x, y;

    if ( state->grid->type != MINES_GRID_SQUARE &&
         state->grid->type != MINES_GRID_SQUARE_CYCLIC &&
         state->grid->type != MINES_GRID_OCTAGONAL2)
        return NULL;

    ret = snewn((state->w + 1) * state->h + 1, char);
    for (y = 0; y < state->h; y++) {
	for (x = 0; x < state->w; x++) {
	    int v = state->board[y*state->w+x];
	    if (v == 0)
		v = '-';
	    else if (v >= 1 && v <= MAX_NEIGHBOURS)
		v = '0' + v;   /* N.B. If printing other grids, check
                                  if two digits are needed! */
	    else if (v == -1)
		v = '*';
	    else if (v == -2 || v == -3)
		v = '?';
	    else if (v >= 64)
		v = '!';
	    ret[y * (state->w+1) + x] = v;
	}
	ret[y * (state->w+1) + state->w] = '\n';
    }
    ret[(state->w + 1) * state->h] = '\0';

    return ret;
}

/* ----------------------------------------------------------------------
 * Input handling
 */

struct game_ui {
    int htile;	               /* for mouse-down highlights */
    bool hneighbour;	       /* for mouse-down highlights */
    bool validneighbour;
    bool flash_is_death;
    int deaths;
    bool completed;
    int cur_tile;
    bool cur_visible;
    bool highlight_flags;
    bool flashed;
};

static game_ui *new_ui(const game_state *state)
{
    game_ui *ui = snew(game_ui);
    ui->htile = -1;
    ui->hneighbour = false;
    ui->validneighbour = false;
    ui->deaths = 0;
    ui->completed = false;
    ui->flash_is_death = false;	       /* *shrug* */
    ui->cur_tile = 0;
    ui->cur_visible = getenv_bool("PUZZLES_SHOW_CURSOR", false) &&
        state->grid->has_cursor;
    ui->highlight_flags = false;
    ui->flashed = false;
    return ui;
}

static config_item *get_prefs(game_ui *ui)
{
    config_item *cfg;

    cfg = snewn(N_PREF_ITEMS+1, config_item);

    cfg[PREF_HIGHLIGHT_FLAGS].name =
        "Highlight adjacent flags when clicking on a clue";
    cfg[PREF_HIGHLIGHT_FLAGS].kw = "highlight-flags";
    cfg[PREF_HIGHLIGHT_FLAGS].type = C_BOOLEAN;
    cfg[PREF_HIGHLIGHT_FLAGS].u.boolean.bval = ui->highlight_flags;

    cfg[N_PREF_ITEMS].name = NULL;
    cfg[N_PREF_ITEMS].type = C_END;

    return cfg;
}

static void set_prefs(game_ui *ui, const config_item *cfg)
{
    ui->highlight_flags = cfg[PREF_HIGHLIGHT_FLAGS].u.boolean.bval;
}

static void free_ui(game_ui *ui)
{
    sfree(ui);
}

static char *encode_ui(const game_ui *ui)
{
    char buf[80];
    /*
     * The deaths counter and completion status need preserving
     * across a serialisation.
     */
    sprintf(buf, "D%d", ui->deaths);
    if (ui->completed)
	strcat(buf, "C");
    return dupstr(buf);
}

static void decode_ui(game_ui *ui, const char *encoding,
                      const game_state *state)
{
    int p= 0;
    sscanf(encoding, "D%d%n", &ui->deaths, &p);
    if (encoding[p] == 'C')
	ui->completed = true;
}

static void game_changed_state(game_ui *ui, const game_state *oldstate,
                               const game_state *newstate)
{
    if (newstate->won)
	ui->completed = true;
}

static const char *current_key_label(const game_ui *ui,
                                     const game_state *state, int button)
{
    int ctile = ui->cur_tile;
    int v = state->board[ctile];

    if (state->dead || state->won || !ui->cur_visible) return "";
    if (button == CURSOR_SELECT2) {
        if (v == -2) return "Mark";
        if (v == -1) return "Unmark";
        return "";
    }
    if (button == CURSOR_SELECT) {
        int j, ni, n = 0;
        if (v == -2 || v == -3) return "Uncover";
        if (v == 0) return "";
        /* Count mine markers. */
        for (j = 0; j < state->grid->nsize; j++) {
            ni = state->grid->neighbours[ctile*state->grid->nsize + j];
            if (ni >= 0 && state->board[ni] == -1)
                n++;
        }
        if (n == v) return "Clear";
    }
    return "";
}

struct game_drawstate {
    int w, h, tilesize, bg;
    int xoff, yoff;  /* offset in grid coordinates */
    bool started;
    bool flashed;
    signed char *board;
    /*
     * Items in this `board' array have all the same values as in
     * the game_state board, and in addition:
     * 
     * 	- -10 means the tile was drawn `specially' as a result of a
     * 	  flash, so it will always need redrawing.
     * 
     * 	- -22 and -23 mean the tile is highlighted for a possible
     * 	  click.
     */
    struct grid_info *grid;
    int cur_tile; /* -1 for no cursor displayed. */
    int bb_xmin, bb_xmax, bb_ymin, bb_ymax;

    /* Scratch arrays used inside game_redraw, allocated once up front */
    bool *open;
    int *newboard;
};

/* Helper function for get_face. */
static inline bool inside_face(struct grid_face *f, int xp, int yp)
{
    int j, winding = 0;

    for (j = 0; j < f->order; j++) {
        int jp1 = (j + 1)%f->order;
        int x0 = f->dots[j]->x;
        int y0 = f->dots[j]->y;
        int x1 = f->dots[jp1]->x;
        int y1 = f->dots[jp1]->y;

        if ((y0 <= yp) != (y1 <= yp) &&
            x0 + (yp-y0) * (x1-x0) / (y1-y0) < xp)
            winding += (y0 <= yp) ? -1 : +1;
    }
    return winding != 0;
}

static int get_face(int x, int y, const game_drawstate *ds)
{
    int xp = SCREEN2GRIDX(x);
    int yp = SCREEN2GRIDY(y);
    int i;
    for (i = 0; i < ds->grid->ntiles; i++)
        if (inside_face(ds->grid->game_grid->faces[i], xp, yp))
            return i;

    return -1;
}

/*
 * mines_move_cursor is a modified copy of move_cursor in misc.c.  The
 * reason for not using the original is that we are using index of
 * tiles and not x and y as coordinates.
 */
static char *mines_move_cursor(const struct game_drawstate *ds, int button, int *tile, bool *visible)
{
    int newtile;
    assert(0 <= *tile && *tile < ds->grid->ntiles);
    switch (button) {
    case CURSOR_UP:         newtile = ds->grid->cursor_up[*tile]; break;
    case CURSOR_DOWN:       newtile = ds->grid->cursor_down[*tile]; break;
    case CURSOR_RIGHT:      newtile = ds->grid->cursor_right[*tile]; break;
    case CURSOR_LEFT:       newtile = ds->grid->cursor_left[*tile]; break;
    default: return MOVE_UNUSED;
    }
    if (newtile < 0)
        return MOVE_NO_EFFECT;
#ifdef GRID_DIAGNOSTICS
    printf("New cursor position: %d (from %d)\n", newtile, *tile);
#endif
    *tile = newtile;
    if (visible != NULL && !*visible)
        *visible = true;
    return MOVE_UI_UPDATE;
}

static char *interpret_move(const game_state *from, game_ui *ui,
                            const game_drawstate *ds,
                            int x, int y, int button)
{
    int ctile;
    char buf[256];

    if (from->dead || from->won)
	return NULL;		       /* no further moves permitted */

    ctile = get_face(x, y, ds);

    if (IS_CURSOR_MOVE(button) && ds->grid->has_cursor)
        return mines_move_cursor(ds, button, &ui->cur_tile, &ui->cur_visible);
    if (IS_CURSOR_SELECT(button) && ds->grid->has_cursor) {
        int v = from->board[ui->cur_tile];

        if (!ui->cur_visible) {
            ui->cur_visible = true;
            return MOVE_UI_UPDATE;
        }
        if (button == CURSOR_SELECT2) {
            /* As for RIGHT_BUTTON; only works on covered square. */
            if (v != -2 && v != -1)
                return MOVE_NO_EFFECT;
            if (ds->grid->type == MINES_GRID_SQUARE)
                sprintf(buf, "F%d,%d", ui->cur_tile%ds->w, ui->cur_tile/ds->h);
            else
                sprintf(buf, "F%d", ui->cur_tile);
            return dupstr(buf);
        }
        /* Otherwise, treat as LEFT_BUTTON, for a single square. */
        if (v == -2 || v == -3) {
            if (from->layout->mines &&
                from->layout->mines[ui->cur_tile])
                ui->deaths++;

            if (ds->grid->type == MINES_GRID_SQUARE)
                sprintf(buf, "O%d,%d", ui->cur_tile%ds->w, ui->cur_tile/ds->h);
            else
                sprintf(buf, "O%d", ui->cur_tile);
            return dupstr(buf);
        }
        ctile = ui->cur_tile;
        ui->validneighbour = true;
        goto uncover;
    }

    if (button == LEFT_BUTTON || button == LEFT_DRAG ||
	button == MIDDLE_BUTTON || button == MIDDLE_DRAG) {
	if (ctile < 0)
	    return MOVE_UNUSED;

	/*
	 * Mouse-downs and mouse-drags just cause highlighting
	 * updates.
	 */
	ui->htile = ctile;
	ui->hneighbour = from->board[ctile] >= 0;
	if (button == LEFT_BUTTON)
	    ui->validneighbour = ui->hneighbour;
	else if (button == MIDDLE_BUTTON)
	    ui->validneighbour = true;
        ui->cur_visible = false;
	return MOVE_UI_UPDATE;
    }

    if (button == RIGHT_BUTTON) {
	if (ctile < 0)
	    return MOVE_UNUSED;

	/*
	 * Right-clicking only works on a covered square, and it
	 * toggles between -1 (marked as mine) and -2 (not marked
	 * as mine).
	 *
	 * FIXME: question marks.
	 */
	if (from->board[ctile] != -2 &&
	    from->board[ctile] != -1)
	    return MOVE_NO_EFFECT;

        if (ds->grid->type == MINES_GRID_SQUARE)
            sprintf(buf, "F%d,%d", ctile % ds->w, ctile/ds->w);
        else
            sprintf(buf, "F%d", ctile);
	return dupstr(buf);
    }

    if (button == LEFT_RELEASE || button == MIDDLE_RELEASE) {
	ui->htile = -1;
	ui->hneighbour = false;

	/*
	 * At this stage we must never return MOVE_UNUSED or
	 * MOVE_NO_EFFECT: we have adjusted the ui, so at worst we
	 * return MOVE_UI_UPDATE.
	 */
	if (ctile < 0)
	    return MOVE_UI_UPDATE;

	/*
	 * Left-clicking on a covered square opens a tile. Not
	 * permitted if the tile is marked as a mine, for safety.
	 * (Unmark it and _then_ open it.)
	 */
	if (button == LEFT_RELEASE &&
	    (from->board[ctile] == -2 ||
	     from->board[ctile] == -3) &&
	    !ui->validneighbour) {
	    /* Check if you've killed yourself. */
	    if (from->layout->mines && from->layout->mines[ctile])
		ui->deaths++;

            if (ds->grid->type == MINES_GRID_SQUARE)
                sprintf(buf, "O%d,%d", ctile%ds->w, ctile/ds->w);
            else
                sprintf(buf, "O%d", ctile);
	    return dupstr(buf);
	}
        goto uncover;
    }
    return MOVE_UNUSED;

uncover:
    {
	/*
	 * Left-clicking or middle-clicking on an uncovered tile:
	 * first we check to see if the number of mine markers
	 * surrounding the tile is equal to its mine count, and if
	 * so then we open all other surrounding squares.
	 */
	if (from->board[ctile] > 0 && ui->validneighbour) {
	    int j, ni, n;

	    /* Count mine markers. */
	    n = 0;
            for (j = 0; j < ds->grid->nsize; j++) {
                ni = ds->grid->neighbours[ctile*ds->grid->nsize + j];
                if ( ni >= 0 && from->board[ni] == -1)
                    n++;
            }

	    if (n == from->board[ctile]) {

		/*
		 * Now see if any of the squares we're clearing
		 * contains a mine (which will happen iff you've
		 * incorrectly marked the mines around the clicked
		 * square). If so, we open _just_ those squares, to
		 * reveal as little additional information as we
		 * can.
		 */
		char *p = buf;
		const char *sep = "";

                for (j = 0; j < ds->grid->nsize; j++) {
                    ni = ds->grid->neighbours[ctile*ds->grid->nsize + j];
                    if (ni >= 0 &&
                        from->board[ni] != -1 &&
                        from->layout->mines &&
                        from->layout->mines[ni]) {
                        if (ds->grid->type == MINES_GRID_SQUARE)
                            p += sprintf(p, "%sO%d,%d", sep, ni%ds->w, ni/ds->w);
                        else
                            p += sprintf(p, "%sO%d", sep, ni);
                        sep = ";";
                    }
                }

		if (p > buf) {
		    ui->deaths++;
		} else {
                    if (ds->grid->type == MINES_GRID_SQUARE)
                        sprintf(buf, "C%d,%d", ctile%ds->w, ctile/ds->w);
                    else
                        sprintf(buf, "C%d", ctile);
		}

		return dupstr(buf);
	    }
	}

	return MOVE_UI_UPDATE;
    }
}

static game_state *execute_move(const game_state *from, const char *move)
{
    game_state *ret;

    if (!strcmp(move, "S")) {
	int ii;

        if (!from->layout->mines) return NULL; /* Game not started. */
	ret = dup_game(from);
        if (!ret->dead) {
            /*
             * If the player is still alive at the moment of pressing
             * Solve, expose the entire board as if it were a completed
             * solution.
             */
            for (ii = 0; ii < from->grid->ntiles; ii++)
                if (ret->layout->mines[ii]) {
                    ret->board[ii] = -1;
                } else {
                    int j, ni, v;

                    v = 0;

                    for (j = 0; j < from->grid->nsize; j++) {
                        ni = from->grid->neighbours[ii * from->grid->nsize + j];
                        if (ni >= 0 && ret->layout->mines[ni])
                            v++;
                    }
                    ret->board[ii] = v;
                }
        } else {
            /*
             * If the player pressed Solve _after dying_, show a full
             * corrections board in the style of standard Minesweeper.
             * Players who don't like Mines's behaviour on death of
             * only showing the mine that killed you (so that in case
             * of a typo you can undo and carry on without the rest of
             * the board being spoiled) can use this to get the display
             * that ordinary Minesweeper would have given them.
             */
            for (ii = 0; ii < from->grid->ntiles; ii++) {
                if ((ret->board[ii] == -2 || ret->board[ii] == -3) &&
                    ret->layout->mines[ii]) {
                    ret->board[ii] = 64;
                } else if (ret->board[ii] == -1 &&
                           !ret->layout->mines[ii]) {
                    ret->board[ii] = 66;
                }
            }
        }
        ret->used_solve = true;

	return ret;
    } else {
        /* Dead players should stop trying to move. */
        if (from->dead)
            return NULL;
	ret = dup_game(from);

	while (*move) {
            if (from->grid->type == MINES_GRID_SQUARE) {
                int cx, cy;

                if (move[0] == 'F' &&
                    sscanf(move+1, "%d,%d", &cx, &cy) == 2 &&
                    cx >= 0 && cx < from->w && cy >= 0 && cy < from->h &&
                    (ret->board[cy * from->w + cx] == -1 ||
                     ret->board[cy * from->w + cx] == -2)) {
                    ret->board[cy * from->w + cx] ^= (-2 ^ -1);
                } else if (move[0] == 'O' &&
                           sscanf(move+1, "%d,%d", &cx, &cy) == 2 &&
                           cx >= 0 && cx < from->w && cy >= 0 && cy < from->h) {
                    open_square(ret, cy*from->w + cx);
                } else if (move[0] == 'C' &&
                           sscanf(move+1, "%d,%d", &cx, &cy) == 2 &&
                           cx >= 0 && cx < from->w && cy >= 0 && cy < from->h) {
                    int dx, dy;

                    for (dy = -1; dy <= +1; dy++)
                        for (dx = -1; dx <= +1; dx++)
                            if (cx+dx >= 0 && cx+dx < ret->w &&
                                cy+dy >= 0 && cy+dy < ret->h &&
                                (ret->board[(cy+dy)*ret->w+(cx+dx)] == -2 ||
                                 ret->board[(cy+dy)*ret->w+(cx+dx)] == -3))
                                open_square(ret, (cy+dy)*from->w + cx+dx);
                } else {
                    free_game(ret);
                    return NULL;
                }
            } else {
                int ctile;
                if (move[0] == 'F' &&
                    sscanf(move+1, "%d", &ctile) == 1 &&
                    ctile >= 0 && ctile < from->grid->ntiles &&
                    (ret->board[ctile] == -1 ||
                     ret->board[ctile] == -2)) {
                    ret->board[ctile] ^= (-2 ^ -1);
                } else if (move[0] == 'O' &&
                           sscanf(move+1, "%d", &ctile) == 1 &&
                           ctile >= 0 &&  ctile < from->grid->ntiles) {
                    open_square(ret, ctile);
                } else if (move[0] == 'C' &&
                           sscanf(move+1, "%d", &ctile) == 1 &&
                           ctile >= 0 &&  ctile < from->grid->ntiles) {
                    int j;
                    for (j = 0; j < from->grid->nsize; j++) {
                        int ni = from->grid->neighbours[ctile*from->grid->nsize + j];
                        if (ni >= 0 &&
                            (ret->board[ni] == -2 ||
                             ret->board[ni] == -3))
                            open_square(ret, ni);
                    }
                } else {
                    free_game(ret);
                    return NULL;
                }
            }

	    while (*move && *move != ';') move++;
	    if (*move) move++;
	}

	return ret;
    }
}

/* ----------------------------------------------------------------------
 * Drawing routines.
 */

static void game_compute_size(const game_params *params, int tilesize,
                              const game_ui *ui, int *x, int *y)
{
    int grid_width, grid_height, rendered_width, rendered_height;
    int g_tilesize;

    /* Ick: fake up `ds->tilesize' for macro expansion purposes */
    struct { int tilesize; } ads, *ds = &ads;
    ads.tilesize = tilesize;

    grid_compute_size(grid_types[params->type], params->w, params->h,
                      &g_tilesize, &grid_width, &grid_height);

    g_tilesize = round(g_tilesize * grid_scale[params->type]);

    /* multiply first to minimise rounding error on integer division */
    rendered_width = grid_width * tilesize / g_tilesize;
    rendered_height = grid_height * tilesize / g_tilesize;
    *x = rendered_width + 2 * BORDER;
    *y = rendered_height + 2 * BORDER;
}

static void game_set_size(drawing *dr, game_drawstate *ds,
                          const game_params *params, int tilesize)
{
    ds->tilesize = tilesize;
}

static float *game_colours(frontend *fe, int *ncolours)
{
    float *ret = snewn(3 * NCOLOURS, float);

    frontend_default_colour(fe, &ret[COL_BACKGROUND * 3]);

    ret[COL_BACKGROUND2 * 3 + 0] = ret[COL_BACKGROUND * 3 + 0] * 19.0F / 20.0F;
    ret[COL_BACKGROUND2 * 3 + 1] = ret[COL_BACKGROUND * 3 + 1] * 19.0F / 20.0F;
    ret[COL_BACKGROUND2 * 3 + 2] = ret[COL_BACKGROUND * 3 + 2] * 19.0F / 20.0F;

    ret[COL_1 * 3 + 0] = 0.0F;
    ret[COL_1 * 3 + 1] = 0.0F;
    ret[COL_1 * 3 + 2] = 1.0F;

    ret[COL_2 * 3 + 0] = 0.0F;
    ret[COL_2 * 3 + 1] = 0.5F;
    ret[COL_2 * 3 + 2] = 0.0F;

    ret[COL_3 * 3 + 0] = 1.0F;
    ret[COL_3 * 3 + 1] = 0.0F;
    ret[COL_3 * 3 + 2] = 0.0F;

    ret[COL_4 * 3 + 0] = 0.0F;
    ret[COL_4 * 3 + 1] = 0.0F;
    ret[COL_4 * 3 + 2] = 0.5F;

    ret[COL_5 * 3 + 0] = 0.5F;
    ret[COL_5 * 3 + 1] = 0.0F;
    ret[COL_5 * 3 + 2] = 0.0F;

    ret[COL_6 * 3 + 0] = 0.0F;
    ret[COL_6 * 3 + 1] = 0.5F;
    ret[COL_6 * 3 + 2] = 0.5F;

    ret[COL_7 * 3 + 0] = 0.0F;
    ret[COL_7 * 3 + 1] = 0.0F;
    ret[COL_7 * 3 + 2] = 0.0F;

    ret[COL_8 * 3 + 0] = 0.5F;
    ret[COL_8 * 3 + 1] = 0.5F;
    ret[COL_8 * 3 + 2] = 0.5F;

    ret[COL_9 * 3 + 0] = 0.5F;
    ret[COL_9 * 3 + 1] = 0.25F;
    ret[COL_9 * 3 + 2] = 0.5F;

    ret[COL_10 * 3 + 0] = 0.5F;
    ret[COL_10 * 3 + 1] = 0.5F;
    ret[COL_10 * 3 + 2] = 0.25F;

    ret[COL_11 * 3 + 0] = 0.25F;
    ret[COL_11 * 3 + 1] = 0.5F;
    ret[COL_11 * 3 + 2] = 0.5F;

    ret[COL_12 * 3 + 0] = 0.25F;
    ret[COL_12 * 3 + 1] = 0.25F;
    ret[COL_12 * 3 + 2] = 0.25F;

    ret[COL_MINE * 3 + 0] = 0.0F;
    ret[COL_MINE * 3 + 1] = 0.0F;
    ret[COL_MINE * 3 + 2] = 0.0F;

    ret[COL_BANG * 3 + 0] = 1.0F;
    ret[COL_BANG * 3 + 1] = 0.0F;
    ret[COL_BANG * 3 + 2] = 0.0F;

    ret[COL_CROSS * 3 + 0] = 1.0F;
    ret[COL_CROSS * 3 + 1] = 0.0F;
    ret[COL_CROSS * 3 + 2] = 0.0F;

    ret[COL_FLAG * 3 + 0] = 1.0F;
    ret[COL_FLAG * 3 + 1] = 0.0F;
    ret[COL_FLAG * 3 + 2] = 0.0F;

    ret[COL_FLAGLIGHT * 3 + 0] = 1.0F;
    ret[COL_FLAGLIGHT * 3 + 1] = 1.0F;
    ret[COL_FLAGLIGHT * 3 + 2] = 0.0F;

    ret[COL_FLAGBASE * 3 + 0] = 0.0F;
    ret[COL_FLAGBASE * 3 + 1] = 0.0F;
    ret[COL_FLAGBASE * 3 + 2] = 0.0F;

    ret[COL_QUERY * 3 + 0] = 0.0F;
    ret[COL_QUERY * 3 + 1] = 0.0F;
    ret[COL_QUERY * 3 + 2] = 0.0F;

    ret[COL_HIGHLIGHT * 3 + 0] = 1.0F;
    ret[COL_HIGHLIGHT * 3 + 1] = 1.0F;
    ret[COL_HIGHLIGHT * 3 + 2] = 1.0F;

    ret[COL_LOWLIGHT * 3 + 0] = ret[COL_BACKGROUND * 3 + 0] * 2.0F / 3.0F;
    ret[COL_LOWLIGHT * 3 + 1] = ret[COL_BACKGROUND * 3 + 1] * 2.0F / 3.0F;
    ret[COL_LOWLIGHT * 3 + 2] = ret[COL_BACKGROUND * 3 + 2] * 2.0F / 3.0F;

    ret[COL_WRONGNUMBER * 3 + 0] = 1.0F;
    ret[COL_WRONGNUMBER * 3 + 1] = 0.6F;
    ret[COL_WRONGNUMBER * 3 + 2] = 0.6F;

    /* Red tinge to a light colour, for the cursor. */
    ret[COL_CURSOR * 3 + 0] = ret[COL_HIGHLIGHT * 3 + 0];
    ret[COL_CURSOR * 3 + 1] = ret[COL_HIGHLIGHT * 3 + 0] / 2.0F;
    ret[COL_CURSOR * 3 + 2] = ret[COL_HIGHLIGHT * 3 + 0] / 2.0F;

    *ncolours = NCOLOURS;
    return ret;
}

static game_drawstate *game_new_drawstate(drawing *dr, const game_state *state)
{
    struct game_drawstate *ds = snew(struct game_drawstate);

    ds->w = state->w;
    ds->h = state->h;
    ds->started = false;
    ds->flashed = false;
    ds->tilesize = 0;                  /* not decided yet */
    ds->xoff = 0;
    ds->yoff = 0;
    ds->board = snewn(state->grid->ntiles, signed char);
    ds->bg = -1;
    ds->cur_tile = -1;
    ds->grid = state->grid;
    state->grid->refcount++;
    ds->open = snewn(state->grid->ntiles, bool);
    ds->newboard = snewn(state->grid->ntiles, int);

    memset(ds->board, -99, state->grid->ntiles);

    return ds;
}

static void game_free_drawstate(drawing *dr, game_drawstate *ds)
{
    if (--ds->grid->refcount <= 0)
        free_grid_info(ds->grid);
    sfree(ds->board);
    sfree(ds->open);
    sfree(ds->newboard);
    sfree(ds);
}

static void calc_highlight_dxy(float xn1, float yn1, float xn2, float yn2,
                               float *dx, float *dy)
{
    *dx = 0;
    *dy = 0;
    if (fabs(xn2*yn1 - xn1*yn2) < 0.01) {
        /* (almost) parallel, just take one of them */
        *dx = xn2;
        *dy = yn2;
    } else {
        *dx = (yn1-yn2) / (xn2*yn1 - xn1*yn2);
        *dy = (xn2-xn1) / (xn2*yn1 - xn1*yn2);
    }
}

/* Used by calc_corner */
struct corner_candidate {
    struct corner_candidate *next;
    int dx;
    int dy;
    float dist2;
};

static void calc_corner(int x1, int y1, int x2, int y2, int x3, int y3,
                        int *xc, int *yc)
{
    float dx, dy;
    /*
     * Include the square of (2*n)^2 pixels as candidates.
     * Unfortunately, this is resolution dependent.  By shrinking the
     * window it is possible to force a failure on several grids with
     * narrow angles even for rather high values of n but those
     * resolutions are not playable!  In those cases we use a fallback
     * to give an illusion of functionality.  With n = 6 this happens
     * when the sides of the tile is about a pixel in length.  Long
     * before that the chosen corner is probably outside of the tile
     * away from the corner of investigation.
     *
     * The Penrose Rhombs grid is the grid with the narrowest angles
     * (36 degrees) and good to use for testing this function.
     */
    int n = 6;
    /*
     * Offset for the virtual vertices used to determine the real
     * corners of the tiles.  The purpose is not to exclude the pixel
     * at the vertex from being a corner in a tile.
     */
    const float xoff = -0.5, yoff = -0.5;
    /*
     * Move the sides inwards to make sure the tile doesn't overwrite
     * any neighbouring tile.  This should in principle be about half
     * a pixel to make sure the edges of the tiles do not overwrite
     * each other.  However, this leads to a corner chosen needlessly
     * far away in many cases.  A smaller safety distance gives a
     * better result.
     */
    const float halfdist = 0.1;
    /* We save the list of candidates between calls. */
    static struct corner_candidate *cc = NULL;
    struct corner_candidate *find;
    float x, y, xn1, yn1, xn2, yn2;

    if (cc == NULL) {
        /* Fill in the corner cancidate list */
        int dx, dy;
        struct corner_candidate *newcc;
        for (dy = -n; dy < n; dy++)
            for (dx = -n; dx < n; dx++) {
                newcc = snew(struct corner_candidate);
                newcc->dx = dx;
                newcc->dy = dy;
                newcc->dist2 = (dx-xoff)*(dx-xoff) + (dy-yoff)*(dy-yoff);
                /* Insert it sorted in the list */
                if (cc == NULL || cc->dist2 > newcc->dist2) {
                    newcc->next = cc;
                    cc = newcc;
                } else {
                    find = cc;
                    while (true) {
                        if (find->next == NULL ||
                            find->next->dist2 > newcc->dist2) {
                            newcc->next = find->next;
                            find->next = newcc;
                            break;
                        }
                        find = find->next;
                    }
                }
            }
    }

    /* Inwards unit normals */
    x = y1-y2;
    y = x2-x1;
    xn1 = x / sqrtf(x*x+y*y);
    yn1 = y / sqrtf(x*x+y*y);

    x = y2-y3;
    y = x3-x2;
    xn2 = x / sqrtf(x*x+y*y);
    yn2 = y / sqrtf(x*x+y*y);


    if (fabs(xn2*yn1 - xn1*yn2) < 0.01) {
        /*
         * (Almost) parallel, just take one of them.  This could also
         * happen if the directions are (almost) opposit.  This
         * happens for the Penrose Rhombs grid in such low resolutions
         * that the sides are about a pixel in length.
         */
        dx = xn2;
        dy = yn2;
    } else {
        dx = (yn1-yn2) / (xn2*yn1 - xn1*yn2);
        dy = (xn2-xn1) / (xn2*yn1 - xn1*yn2);
    }

    /*
     * Go through the candidate list and find the first one inside the
     * tile.  Inside is determined by being to the right of the edges
     * p1->p2 and p2->p3.
     */
    for (find = cc; find != NULL; find = find->next) {
        if ((find->dx - xoff - halfdist*dx) * (y2 - y1)
            - (x2 - x1) * (find->dy - yoff - halfdist*dy) <= 0 &&
            (find->dx - xoff - halfdist*dx) * (y3 - y2)
            - (x3 - x2) * (find->dy - yoff - halfdist*dy) <= 0) {
            *xc = x2 + find->dx;
            *yc = y2 + find->dy;
            return;
        }
    }
#ifdef GRID_DIAGNOSTICS
    /*
     * Include this assert only for developers.  Use a fallback
     * otherwise.  See comment at the beginning of the present
     * function.
     */
    printf("calc_corner (x1 y1) = (%d %d), (x2 y2) = (%d %d), (x3 y3) = (%d %d)\n",
           x1, y1, x2, y2, x3, y3);
    printf("calc_corner dx = %g, dy = %g\n", dx, dy);
    printf("calc_corner xn1 = %g, yn1 = %g xn2 = %g, yn2 = %g\n", xn1, yn1, xn2, yn2);
    assert(!"No corner found in corner candidate list");
#endif
    /* Fallback if the above failed. */
    *xc = x2 + round(dx);
    *yc = y2 + round(dy);
}

static void draw_tile(drawing *dr, game_drawstate *ds, const game_ui *ui,
                      int tile, int v, int bg, const signed char *board,
                      bool *open)
{
    int hl = 0;
    int i;
    int order = ds->grid->game_grid->faces[tile]->order;
    int *coords = ds->grid->tiles[tile].coords;
    int ix, iy;
    float sf;

    assert(order <= MAX_EDGES);
    if ( v >= 0 || v == -22 || v == -23 || v == -24) {
        /*
	 * Clear the square to the background colour, and draw thin
	 * grid lines.
	 * 
	 * Exception is that for value 65 (mine we've just trodden
	 * on), we clear the square to COL_BANG.
	 */
	if (v == -22 || v == -23 || v == -24) {
	    v += 20;
            bg = bg == COL_BACKGROUND ? COL_BACKGROUND2 : bg;
        } else {
            if (v & 32) {
                bg = COL_WRONGNUMBER;
                v &= ~32;
            }
            bg = v == 65 ? COL_BANG :
                bg == COL_BACKGROUND ? COL_BACKGROUND2 : bg;

        }
        if (bg != COL_BACKGROUND) /* We have already cleared the area! */
            draw_polygon(dr, coords, order, bg, bg);

        grid_face *f = ds->grid->game_grid->faces[tile];
        if (ds->grid->type == MINES_GRID_SQUARE ||
            ds->grid->type == MINES_GRID_SQUARE_CYCLIC) {
            /* The Square grid needs the outlines always not to look strange */
            for (i = 0; i < 4; i+=3) /* Yak!  We want i = 0 and i = 3 */
                draw_line(dr, GRID2SCREENX(f->edges[i]->dot1->x),
                          GRID2SCREENY(f->edges[i]->dot1->y),
                          GRID2SCREENX(f->edges[i]->dot2->x),
                          GRID2SCREENY(f->edges[i]->dot2->y), COL_LOWLIGHT);
        } else {
            /*
             * Draw outline if neighbour tile is empty too.  This test
             * is needed since the tiles might not fit perfectly to
             * the outline.  Drawing the outlines anyway would then
             * make it look skewed.  It may still sometimes look like
             * a tiles tip don't go all the way to the outline but it
             * is hard to do something about that.
             */
            for (i = 0; i < order; i++) {
                grid_face *other = (f->edges[i]->face1 == f)? f->edges[i]->face2: f->edges[i]->face1;
                if (other == NULL || other->index > tile)
                    continue;
                if (!open[other->index])
                    continue;
                /* Use original grid coordinates */
                draw_line(dr, GRID2SCREENX(f->edges[i]->dot1->x),
                          GRID2SCREENY(f->edges[i]->dot1->y),
                          GRID2SCREENX(f->edges[i]->dot2->x),
                          GRID2SCREENY(f->edges[i]->dot2->y), COL_LOWLIGHT);
            }
        }
    } else {
        /*
         * Draw highlights to indicate the square is covered.
         */
        int innercoords[2*MAX_EDGES];
        int shades[2*4];
        int col;

        float x, y;
        float xn[MAX_EDGES], yn[MAX_EDGES];
        float dx, dy;

        for (i = 0; i < order; i++) {
            int ip = (i+1)%order;
            x = coords[2*i+1] - coords[2*ip+1];  /* inwards normal */
            y = coords[2*ip] - coords[2*i];
            xn[i] = x /sqrtf(x*x+y*y);
            yn[i] = y /sqrtf(x*x+y*y);
        }

        for (i = 0; i < order; i++) {
            int im = (i-1+order)%order;
            float dxh, dyh;
            calc_highlight_dxy(xn[im], yn[im], xn[i], yn[i], &dxh, &dyh);

            dx = dxh*HIGHLIGHT_WIDTH;
            dy = dyh*HIGHLIGHT_WIDTH;
            float len2 = dx*dx + dy*dy;
            if (len2 < 1) {
                dx /= sqrtf(len2);
                dy /= sqrtf(len2);
            }
            innercoords[2*i] = coords[2*i] + round(dx);
            innercoords[2*i+1] = coords[2*i+1] + round(dy);
        }

        for (i = 0; i < order; i++) {
            int ip = (i+1)%order;
            shades[0] = coords[2*i];
            shades[1] = coords[2*i+1];
            shades[2] = coords[2*ip];
            shades[3] = coords[2*ip+1];
            shades[4] = innercoords[2*ip];
            shades[5] = innercoords[2*ip+1];
            shades[6] = innercoords[2*i];
            shades[7] = innercoords[2*i+1];
            grid_face *f = ds->grid->game_grid->faces[tile];
            if ((f->dots[i]->y - f->dots[ip]->y)*xlight
                + (f->dots[ip]->x - f->dots[i]->x)*ylight > 0) {
                col = COL_LOWLIGHT ^ hl;
            } else {
                col = COL_HIGHLIGHT ^ hl;
            }
            draw_polygon(dr, shades, 4, col, col);
        }

        draw_polygon(dr, innercoords, order,
                     bg, bg);
    }

    ix = GRID2SCREENX(ds->grid->game_grid->faces[tile]->ix);
    iy = GRID2SCREENY(ds->grid->game_grid->faces[tile]->iy);

    /*
     * When corner coordinates are calculated for tiles, the right and
     * bottom edges are in principel moved one pixel left/up.  This
     * suggests that the centre point should be moved half a pixel
     * left and up.  There is also an optical illusion at work with
     * the bright and dark highlights that makes rounding off towards
     * left/up is better for closed tiles.  The bombs, drawn on open
     * tiles, look equally wrong either way.  For some reason, the
     * clue numbers on the open tiles looks better with the more
     * right/down version.  Perhaps it is a font issue and could be
     * subject to the choice of frontend.
     *
     * The graphics is well tuned since before for the Square grid so
     * we better leave that as is.
     */
    if (ds->grid->type != MINES_GRID_SQUARE &&
        ds->grid->type != MINES_GRID_SQUARE_CYCLIC &&
        v < 0) {
        ix -= 1;
        iy -= 1;
    }
    sf = min(1., 2. * ds->grid->game_grid->faces[tile]->iradius /
             ds->grid->tilesize);

    if (v == -1 || v == -21) {
        /*
         * Draw a flag.
         */
        int flagcoords[12];

#define SETCOORD(n, dx, dy) do {                        \
    flagcoords[(n)*2+0] = ix + (int)(sf * TILE_SIZE * (dx)); \
    flagcoords[(n)*2+1] = iy + (int)(sf * TILE_SIZE * (dy)); \
} while (0)
        if (ds->grid->type == MINES_GRID_SQUARE ||
            ds->grid->type == MINES_GRID_SQUARE_CYCLIC) {
            SETCOORD(0,  0.1F,  -0.15F);
            SETCOORD(1,  0.1F,   0.2F);
            SETCOORD(2,  0.3F,   0.3F);
            SETCOORD(3, -0.25F,  0.3F);
            SETCOORD(4,  0.05F,  0.2F);
            SETCOORD(5,  0.05F, -0.15F);
            draw_polygon(dr, flagcoords, 6, COL_FLAGBASE, COL_FLAGBASE);

            SETCOORD(0,  0.1F, -0.3F);
            SETCOORD(1,  0.1F,  0.0F);
            SETCOORD(2, -0.3F, -0.15F);
            if (v == -21 && ui->highlight_flags)
                draw_polygon(dr, flagcoords, 3, COL_FLAGLIGHT, COL_FLAG);
            else
                draw_polygon(dr, flagcoords, 3, COL_FLAG, COL_FLAG);
        } else {
            /*
             * This flag does not stick out as much in the lower right
             * corner.  We also offset it slightly upwards to better
             * fit in the inscribed circle in tiles with different
             * shapes.
             */
            SETCOORD(0,  0.1F,  -0.15F-0.02F);
            SETCOORD(1,  0.1F,   0.2F -0.02F);
            SETCOORD(2,  0.25F,  0.3F -0.02F);
            SETCOORD(3, -0.25F,  0.3F -0.02F);
            SETCOORD(4,  0.05F,  0.2F -0.02F);
            SETCOORD(5,  0.05F, -0.15F-0.02F);
            draw_polygon(dr, flagcoords, 6, COL_FLAGBASE, COL_FLAGBASE);

            SETCOORD(0,  0.1F, -0.3F -0.02F);
            SETCOORD(1,  0.1F,  0.0F -0.02F);
            SETCOORD(2, -0.3F, -0.15F-0.02F);
            if (v == -21 && ui->highlight_flags)
                draw_polygon(dr, flagcoords, 3, COL_FLAGLIGHT, COL_FLAG);
            else
                draw_polygon(dr, flagcoords, 3, COL_FLAG, COL_FLAG);
        }
#undef SETCOORD

    } else if (v == -3) {
        /*
         * Draw a question mark.
         */
        draw_text(dr, ix, iy,
                  FONT_VARIABLE, TILE_SIZE * 6 / 8,
                  ALIGN_VCENTRE | ALIGN_HCENTRE,
                  COL_QUERY, "?");
    } else if (v == -4) {
        /*
         * Draw a 'click here' cross, to mark the safe first click
         * location.
         */
        int c0 = sf * TILE_SIZE / 4, c1 = c0 - sf;
        draw_line(dr, ix-c0, iy-c0, ix+c1, iy+c1, COL_MINE);
        draw_line(dr, ix-c0, iy+c1, ix+c1, iy-c0, COL_MINE);

    } else if (v > 0 && v <= MAX_NEIGHBOURS) {
        /*
         * Mark a number.
         */
        char str[3];
        int fontsize;

        if (v <= 9) {
            str[0] = v + '0';
            str[1] = '\0';
            fontsize = sf * TILE_SIZE * 7 / 8;
        } else {
            str[0] = v/10 + '0';
            str[1] = v%10 + '0';
            str[2] = '\0';
            fontsize = sf * TILE_SIZE * 5 / 8;
        }
        draw_text(dr, ix, iy,
                  FONT_VARIABLE, fontsize,
                  ALIGN_VCENTRE | ALIGN_HCENTRE,
                  (COL_1 - 1) + v, str);

    } else if (v >= 64) {
        /*
         * Mark a mine.
         */
        {
            int r = sf * TILE_SIZE / 2 - 3;
            draw_circle(dr, ix, iy, 5*r/6, COL_MINE, COL_MINE);
            draw_rect(dr, ix - r/6, iy - r, 2*(r/6)+1, 2*r+1, COL_MINE);
            draw_rect(dr, ix - r, iy - r/6, 2*r+1, 2*(r/6)+1, COL_MINE);
            draw_rect(dr, ix-r/3, iy-r/3, r/3, r/4, COL_HIGHLIGHT);
        }

        if (v == 66) {
            /*
             * Cross through the mine.
             */
            int dx;
            int xs = sf*TILE_SIZE/2 - 3;
            int ys = sf*TILE_SIZE/2 - 2;

            for (dx = -1; dx <= +1; dx++) {
                draw_line(dr,
                          ix - xs + dx, iy - ys,
                          ix + xs + dx, iy + ys,
                          COL_CROSS);
                draw_line(dr,
                          ix + xs + dx, iy - ys,
                          ix - xs + dx, iy + ys,
                          COL_CROSS);
            }
        }
    }
}

static void calc_tile_info( struct game_drawstate *ds)
{
    int i, j;
    int xmin, xmax, ymin, ymax;

    for (i = 0; i < ds->grid->ntiles; i++) {
        int order = ds->grid->tiles[i].order;
        for (j = 0; j < order; j++) {
            int jm = (j - 1 + order) % order;
            int jp = (j + 1) % order;
            grid_dot *d1 = ds->grid->game_grid->faces[i]->dots[jm];
            grid_dot *d2 = ds->grid->game_grid->faces[i]->dots[j];
            grid_dot *d3 = ds->grid->game_grid->faces[i]->dots[jp];
            calc_corner(GRID2SCREENX(d1->x),
                        GRID2SCREENY(d1->y),
                        GRID2SCREENX(d2->x),
                        GRID2SCREENY(d2->y),
                        GRID2SCREENX(d3->x),
                        GRID2SCREENY(d3->y),
                        &ds->grid->tiles[i].coords[2*j],
                        &ds->grid->tiles[i].coords[2*j+1]);
        }
        xmin = ymin = INT_MAX;
        xmax = ymax = INT_MIN;
        for (j = 0; j < ds->grid->tiles[i].order; j++) {
            int x = GRID2SCREENX(ds->grid->game_grid->faces[i]->dots[j]->x);
            int y = GRID2SCREENY(ds->grid->game_grid->faces[i]->dots[j]->y);
            xmin = min(xmin, x);
            xmax = max(xmax, x);
            ymin = min(ymin, y);
            ymax = max(ymax, y);
        }
        ds->grid->tiles[i].bb_xmin = xmin - 1;
        ds->grid->tiles[i].bb_xmax = xmax + 1;
        ds->grid->tiles[i].bb_ymin = ymin - 1;
        ds->grid->tiles[i].bb_ymax = ymax + 1;
    }
}

static void calc_rim_info(struct game_drawstate *ds)
{
    int i, j;
    int bb_xmin = INT_MAX, bb_xmax = INT_MIN;
    int bb_ymin = INT_MAX, bb_ymax = INT_MIN;
    grid_dot *d1, *d2, *d3;
    float xn1, yn1, xn2, yn2, dx, dy, length;
    int dxh, dyh, xc, yc;

    /*
     * The algorithm here relies on the grid to have just one
     * connected boundary loop.  If there is a grid introduced with a
     * hole then this routine must be updated.
     */

    for (i = 0; i < ds->grid->nrims; i++) {
        d1 = ds->grid->rim_dots[i%ds->grid->nrims];
        d2 = ds->grid->rim_dots[(i+1)%ds->grid->nrims];
        d3 = ds->grid->rim_dots[(i+2)%ds->grid->nrims];

        /* inward normals */
        xn1 = d1->y - d2->y;
        yn1 = d2->x - d1->x;
        xn2 = d2->y - d3->y;
        yn2 = d3->x - d2->x;
        length = sqrtf(xn1*xn1 + yn1*yn1);
        xn1 /= length;
        yn1 /= length;
        length = sqrtf(xn2*xn2 + yn2*yn2);
        xn2 /= length;
        yn2 /= length;

        calc_highlight_dxy(xn1, yn1, xn2, yn2, &dx, &dy);
        dxh = round(dx * OUTER_HIGHLIGHT_WIDTH);
        dyh = round(dy * OUTER_HIGHLIGHT_WIDTH);
        calc_corner(GRID2SCREENX(d1->x), GRID2SCREENY(d1->y),
                    GRID2SCREENX(d2->x), GRID2SCREENY(d2->y),
                    GRID2SCREENX(d3->x), GRID2SCREENY(d3->y),
                    &xc, &yc);
        ds->grid->rims[i].coords[0] = xc;
        ds->grid->rims[i].coords[1] = yc;
        ds->grid->rims[i].coords[2] = xc - dxh;
        ds->grid->rims[i].coords[3] = yc - dyh;
    }
    for (i = 0; i < ds->grid->nrims; i++) {
        int ip = (i+1)%ds->grid->nrims;
        int xmin, xmax, ymin, ymax;
        ds->grid->rims[i].coords[4] = ds->grid->rims[ip].coords[2];
        ds->grid->rims[i].coords[5] = ds->grid->rims[ip].coords[3];
        ds->grid->rims[i].coords[6] = ds->grid->rims[ip].coords[0];
        ds->grid->rims[i].coords[7] = ds->grid->rims[ip].coords[1];

        xmin = ymin = INT_MAX;
        xmax = ymax = INT_MIN;
        for (j = 0; j < 4; j++) {
            xmin = min(xmin, ds->grid->rims[i].coords[2*j]);
            xmax = max(xmax, ds->grid->rims[i].coords[2*j]);
            ymin = min(ymin, ds->grid->rims[i].coords[2*j+1]);
            ymax = max(ymax, ds->grid->rims[i].coords[2*j+1]);
        }
        ds->grid->rims[i].bb_xmin = xmin - 1;
        ds->grid->rims[i].bb_xmax = xmax + 1;
        ds->grid->rims[i].bb_ymin = ymin - 1;
        ds->grid->rims[i].bb_ymax = ymax + 1;

        bb_xmin = min(bb_xmin, xmin);
        bb_xmax = max(bb_xmax, xmax);
        bb_ymin = min(bb_ymin, ymin);
        bb_ymax = max(bb_ymax, ymax);
    }
    ds->bb_xmin = bb_xmin;
    ds->bb_ymin = bb_ymin;
    ds->bb_xmax = bb_xmax;
    ds->bb_ymax = bb_ymax;
}

static void game_redraw(drawing *dr, game_drawstate *ds,
                        const game_state *oldstate, const game_state *state,
                        int dir, const game_ui *ui,
                        float animtime, float flashtime)
{
    int i;
    int bg;
    int ci = -1;
    bool cmoved;
    int bb_xmin = INT_MAX, bb_xmax = INT_MIN;
    int bb_ymin = INT_MAX, bb_ymax = INT_MIN;

    if (flashtime) {
	int frame = (int)(flashtime / FLASH_FRAME);
        ds->flashed = true;
	if (frame % 2)
	    bg = (ui->flash_is_death ? COL_BACKGROUND : COL_LOWLIGHT);
	else
	    bg = (ui->flash_is_death ? COL_BANG : COL_HIGHLIGHT);
    } else {
	bg = COL_BACKGROUND;
    }

    if (!ds->started) {
        ds->xoff = state->grid->game_grid->lowest_x;
        ds->yoff = state->grid->game_grid->lowest_y;

        calc_tile_info(ds);
        calc_rim_info(ds);

        /* Fill in the bounding box to force a redraw of the entire area. */
        bb_xmin = ds->bb_xmin;
        bb_xmax = ds->bb_xmax;
        bb_ymin = ds->bb_ymin;
        bb_ymax = ds->bb_ymax;

        ds->started = true;
    }

    if (ui->cur_visible) ci = ui->cur_tile;
    cmoved = (ci != ds->cur_tile);

    /*
     * Go through all tiles to find out if anything needs to be
     * redrawn and update the bounding box accordingly.  We need to do
     * this even if the bounding box is already defined because of the
     * array open.
     */
    for (i = 0; i < ds->grid->ntiles; i++) {
        int v = state->board[i];
        bool cc = false;
        ds->open[i] = false;
        if (v >= 0)
            ds->open[i] = true;
        if (v >= 0 && v <= MAX_NEIGHBOURS) {
            /*
             * Count up the flags around this tile, and if
             * there are too _many_, highlight the tile.
             */
            int flags = 0;
            int j, ni;
            for (j = 0; j < ds->grid->nsize; j++) {
                ni = ds->grid->neighbours[i*ds->grid->nsize + j];
                if ( ni >= 0 )
                    if (state->board[ni] == -1)
                        flags++;
            }
            if (flags > v)
                v |= 32;
        }
        if (v == -2 && i == state->layout->start_tile)
            v = -4;                /* 'start here' cross */

        if ((v == -1 || v == -2 || v == -3 || v == -4) &&
            (i == ui->htile ||
             (ui->hneighbour && is_neighbour_of(ds->grid, i, ui->htile)))) {
            v -= 20;
            if (v != -21)
                ds->open[i] = true;
        }

        if (cmoved && /* if cursor has moved, force redraw of curr and prev pos */
            ((i == ci) || (i == ds->cur_tile)))
            cc = true;

        if (ds->board[i] != v || bg != ds->bg || cc) {
            /* This tile needs redrawing, find bounding box */
            bb_xmin = min(bb_xmin, ds->grid->tiles[i].bb_xmin);
            bb_xmax = max(bb_xmax, ds->grid->tiles[i].bb_xmax);
            bb_ymin = min(bb_ymin, ds->grid->tiles[i].bb_ymin);
            bb_ymax = max(bb_ymax, ds->grid->tiles[i].bb_ymax);
        }
        ds->newboard[i] = v;
    }

    /*
     * If we have a bounding box, then update the screen within that
     * box, possibly the whole screen.
     */
    if (bb_xmin < bb_xmax) {
        clip(dr, bb_xmin, bb_ymin, bb_xmax - bb_xmin + 1, bb_ymax - bb_ymin + 1);
        draw_rect(dr, bb_xmin, bb_ymin, bb_xmax - bb_xmin + 1, bb_ymax - bb_ymin + 1, COL_BACKGROUND);

        /* first draw the open tiles... */
        for (i = 0; i < ds->grid->ntiles; i++) {
            if (ds->open[i] &&
                ds->grid->tiles[i].bb_xmin <= bb_xmax &&
                ds->grid->tiles[i].bb_xmax >= bb_xmin &&
                ds->grid->tiles[i].bb_ymin <= bb_ymax &&
                ds->grid->tiles[i].bb_ymax >= bb_ymin) {
                draw_tile(dr, ds, ui, i, ds->newboard[i],
                          (i == ci) ? COL_CURSOR : bg, state->board, ds->open);
                ds->board[i] = ds->newboard[i];
            }
        }

        /* ...then the rim... */
        for (i = 0; i < ds->grid->nrims; i++) {
            if (ds->grid->rims[i].bb_xmin <= bb_xmax &&
                ds->grid->rims[i].bb_xmax >= bb_xmin &&
                ds->grid->rims[i].bb_ymin <= bb_ymax &&
                ds->grid->rims[i].bb_ymax >= bb_ymin) {
                int col;
                int ip = (i+1)%ds->grid->nrims;
                int ipp = (i+2)%ds->grid->nrims;
                if ((ds->grid->rim_dots[ip]->y - ds->grid->rim_dots[ipp]->y) * xlight +
                    (ds->grid->rim_dots[ipp]->x - ds->grid->rim_dots[ip]->x) * ylight > 0)
                    col = COL_HIGHLIGHT;
                else
                    col = COL_LOWLIGHT;
                draw_polygon(dr, ds->grid->rims[i].coords, 4, col, col);
            }
        }

        /* ...and lastly the closed tiles */
        for (i = 0; i < ds->grid->ntiles; i++) {
            if (!ds->open[i] &&
                ds->grid->tiles[i].bb_xmin <= bb_xmax &&
                ds->grid->tiles[i].bb_xmax >= bb_xmin &&
                ds->grid->tiles[i].bb_ymin <= bb_ymax &&
                ds->grid->tiles[i].bb_ymax >= bb_ymin) {

                draw_tile(dr, ds, ui, i, ds->newboard[i],
                          (i == ci) ? COL_CURSOR : bg, state->board, ds->open);
                ds->board[i] = ds->newboard[i];
            }
        }
        unclip(dr);
        draw_update(dr, bb_xmin, bb_ymin,
                    bb_xmax - bb_xmin + 1, bb_ymax - bb_ymin + 1);
    }

    ds->bg = bg;
    ds->cur_tile = ci;

    /*
     * Update the status bar.
     */
    {
	char statusbar[512];
        int mines = 0, markers = 0, closed = 0;
        for (i = 0; i < ds->grid->ntiles; i++) {
            int v = state->board[i];

            if (v < 0)
                closed++;
            if (v == -1)
                markers++;
            if (state->layout->mines && state->layout->mines[i])
                mines++;
        }

        if (!state->layout->mines)
            mines = state->layout->n;

	if (state->dead) {
	    sprintf(statusbar, "DEAD!");
	} else if (state->won) {
            if (mines < state->layout->n) {
                int extra = state->layout->n - mines;
                if (extra == 1)
                    sprintf(statusbar, "1 mine didn't fit!");
                else
                    sprintf(statusbar, "%d mines didn't fit!", extra);
            } else if (state->used_solve) {
                sprintf(statusbar, "Auto-solved.");
            } else {
                sprintf(statusbar, "COMPLETED!");
            }
	} else {
            int safe_closed = closed - mines;
	    sprintf(statusbar, "Marked: %d / %d", markers, mines);
            if (safe_closed > 0 && safe_closed <= ds->grid->nsize) {
                /*
                 * In the situation where there's a very small number
                 * of _non_-mine squares left unopened, it's helpful
                 * to mention that number in the status line, to save
                 * the player from having to count it up
                 * painstakingly. This is particularly important if
                 * the player has turned up the mine density to the
                 * point where game generation resorts to its weird
                 * pathological fallback of a very dense mine area
                 * with a clearing in the middle, because that often
                 * leads to a deduction you can only make by knowing
                 * that there is (say) exactly one non-mine square to
                 * find, and it's a real pain to have to count up two
                 * large numbers of squares and subtract them to get
                 * that value of 1.
                 *
                 * The threshold value of grid->nsize for displaying this
                 * information is because that's the largest number of
                 * non-mine squares that might conceivably fit around
                 * a single central square, and the most likely way to
                 * _use_ this information is to observe that if all
                 * the remaining safe squares are adjacent to _this_
                 * square then everything else can be immediately
                 * flagged as a mine.
                 */
                if (safe_closed == 1) {
                    sprintf(statusbar + strlen(statusbar),
                            " (1 safe space remains)");
                } else {
                    sprintf(statusbar + strlen(statusbar),
                            " (%d safe spaces remain)", safe_closed);
                }
            }
	}
        if (ui->deaths)
            sprintf(statusbar + strlen(statusbar),
                    "  Deaths: %d", ui->deaths);
	status_bar(dr, statusbar);
    }
}

static float game_anim_length(const game_state *oldstate,
                              const game_state *newstate, int dir, game_ui *ui)
{
    return 0.0F;
}

static float game_flash_length(const game_state *oldstate,
                               const game_state *newstate, int dir, game_ui *ui)
{
    if (oldstate->used_solve || newstate->used_solve)
        return 0.0F;

    if (dir > 0 && !oldstate->dead && !oldstate->won) {
	if (newstate->dead) {
	    ui->flash_is_death = true;
	    return 3 * FLASH_FRAME;
	}
	if (newstate->won) {
	    ui->flash_is_death = false;
	    return 2 * FLASH_FRAME;
	}
    }
    return 0.0F;
}

static void game_get_cursor_location(const game_ui *ui,
                                     const game_drawstate *ds,
                                     const game_state *state,
                                     const game_params *params,
                                     int *x, int *y, int *w, int *h)
{
    /* There can not ba a local region of interest for cyclic grids */
    if (params->type != MINES_GRID_SQUARE_CYCLIC &&
        params->type != MINES_GRID_HONEYCOMB_CYCLIC &&
        params->type != MINES_GRID_TRIANGULAR_CYCLIC) {
        if(ui->cur_visible) {
            grid_face *f = ds->grid->game_grid->faces[ui->cur_tile];
            *x = GRID2SCREENX(f->ix);
            *y = GRID2SCREENY(f->iy);
            *w = *h = TILE_SIZE;
        }
    }
}

static int game_status(const game_state *state)
{
    /*
     * We report the game as lost only if the player has used the
     * Solve function to reveal all the mines. Otherwise, we assume
     * they'll undo and continue play.
     */
    return state->won ? (state->used_solve ? -1 : +1) : 0;
}

static bool game_timing_state(const game_state *state, game_ui *ui)
{
    if (state->dead || state->won || ui->completed || !state->layout->mines)
	return false;
    return true;
}

#ifdef COMBINED
#define thegame mines
#endif

const struct game thegame = {
    "Mines", "games.mines", "mines",
    default_params,
    NULL, game_preset_menu,
    decode_params,
    encode_params,
    free_params,
    dup_params,
    true, game_configure, custom_params,
    validate_params,
    new_game_desc,
    validate_desc,
    new_game,
    set_public_desc,
    dup_game,
    free_game,
    true, solve_game,
    true, game_can_format_as_text_now, game_text_format,
    get_prefs, set_prefs,
    new_ui,
    free_ui,
    encode_ui,
    decode_ui,
    NULL, /* game_request_keys */
    game_changed_state,
    current_key_label,
    interpret_move,
    execute_move,
    PREFERRED_TILE_SIZE, game_compute_size, game_set_size,
    game_colours,
    game_new_drawstate,
    game_free_drawstate,
    game_redraw,
    game_anim_length,
    game_flash_length,
    game_get_cursor_location,
    game_status,
    false, false, NULL, NULL,          /* print_size, print */
    true,			       /* wants_statusbar */
    true, game_timing_state,
    BUTTON_BEATS(LEFT_BUTTON, RIGHT_BUTTON) | REQUIRE_RBUTTON,
};

#ifdef STANDALONE_OBFUSCATOR

/*
 * Vaguely useful stand-alone program which translates between
 * obfuscated and clear Mines game descriptions. Pass in a game
 * description on the command line, and if it's clear it will be
 * obfuscated and vice versa. The output text should also be a
 * valid game ID describing the same game. Like this:
 *
 * $ ./mineobfusc 9x9:4,4,mb071b49fbd1cb6a0d5868
 * 9x9:4,4,004000007c00010022080
 * $ ./mineobfusc 9x9:4,4,004000007c00010022080
 * 9x9:4,4,mb071b49fbd1cb6a0d5868
 */

int main(int argc, char **argv)
{
    game_params *p;
    game_state *s;
    char *id = NULL, *desc, *grid;
    const char *err;
    int y, x, tile, ntiles;

    while (--argc > 0) {
        char *p = *++argv;
	if (*p == '-') {
            fprintf(stderr, "%s: unrecognised option `%s'\n", argv[0], p);
            return 1;
        } else {
            id = p;
        }
    }

    if (!id) {
        fprintf(stderr, "usage: %s <game_id>\n", argv[0]);
        return 1;
    }

    desc = strchr(id, ':');
    if (!desc) {
        fprintf(stderr, "%s: game id expects a colon in it\n", argv[0]);
        return 1;
    }
    *desc++ = '\0';

    p = default_params();
    decode_params(p, id);
    err = validate_desc(p, desc);
    if (err) {
        *(desc-1) = ':';
        fprintf(stderr, "%s: %s\n", argv[0], err);
        return 1;
    }
    s = new_game(NULL, p, desc);

    grid = extract_grid_desc((const char **) &desc);

    if (p->type == MINES_GRID_SQUARE) {
        x = atoi(desc);
        while (*desc && *desc != ',') desc++;
        if (*desc) desc++;
        y = atoi(desc);
        while (*desc && *desc != ',') desc++;
        if (*desc) desc++;
        tile = y*p->w + x;
        ntiles = p->w * p->h;
    } else {
        tile = atoi(desc);
        while (*desc && *desc != ',') desc++;
        if (*desc) desc++;
        ntiles = atoi(desc);
        while (*desc && *desc != ',') desc++;
        if (*desc) desc++;
    }

    printf("%s:%s\n", id, describe_layout(p->type, s->layout->mines,
                                          ntiles,
                                          p->w, tile, s->layout->n,
                                          (*desc != 'm'), grid));

    return 0;
}

#endif

/* vim: set shiftwidth=4 tabstop=8: */
