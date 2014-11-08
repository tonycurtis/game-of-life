#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

static const useconds_t default_sleep_update_us = 1e5;  /* micro-seconds */
static const unsigned long default_iterations = -1;  /* update, and update... */
static const int default_visible_y = 60;
static const int default_visible_x = 140;
static const char default_live_cell = '*';
static const char default_dead_cell = ' ';

static useconds_t sleep_update_us;
static unsigned long iterations;
static int visible_y;
static int visible_x;
static char live_cell;
static char dead_cell;

/**
 * currently global full grid size (but shouldn't be global, really)
 */
static int grid_y;
static int grid_x;

/**
 * who I am
 */
static char *progname;

/**
 * allocate memory for an entire grid
 */
static
inline
int **
allocate_grid (void)
{
  int y, x;

  int **g = malloc (sizeof (*g) * grid_y);

  for (y = 0; y < grid_y; y += 1)
    {
      g[y] = malloc (sizeof (**g) * grid_x);
    }

  return g;
}

/**
 * initialize a grid to completely dead
 */
static
inline
void
zero_grid (int **g)
{
  int y, x;

  for (y = 0; y < grid_y; y += 1)
    {
      for (x = 0; x < grid_x; x += 1)
        {
          g[y][x] = 0;
        }
    }
}

/**
 * randomly (and fairly sparsely) populate a grid
 */
static
inline
void
randomize_visible_grid (int **g)
{
  int y, x;

  srand (time (NULL) + getpid ());

  for (y = 1; y < grid_y - 1; y += 1)
    {
      for (x = 0; x < grid_x; x += 1)
        {
          g[y][x] = (rand () % 1000) > 900 ? 1 : 0;
        }
    }
}

/**
 * read in a grid population from a file.  Format
 *
 * Y X
 *
 * to make alive, one pair per line
 */
static
inline
void
load_visible_grid (char *filename, int **g)
{
  FILE *fp = fopen (filename, "r");
  char buf[64];

  while (fgets (buf, sizeof (buf) - 1, fp) != NULL)
    {
      int y, x;
      sscanf (buf, "%d %d", &y, &x);
      fprintf (stderr, "Loaded y = %d, x = %d\n", y, x);
      g[y][x] = 1;
    }

  fclose (fp);
}

static
inline
void
update_visible_grid (int **old, int **new)
{
  int y, x;

#pragma omp parallel for private(x)
  for (y = 1; y < grid_y - 1; y += 1)
    {
      for (x = 1; x < grid_x - 1; x += 1)
        {
          /* add up neighbors from above, self, and below */
          const int prn = old[y-1][x-1] + old[y-1][x] + old[y-1][x+1];
          const int srn = old[y  ][x-1]               + old[y  ][x+1];
          const int nrn = old[y+1][x-1] + old[y+1][x] + old[y+1][x+1];
          const int n = prn + srn + nrn;

          if (old[y][x]) /* currently alive */
            {
              new[y][x] = ((n == 2) || (n == 3)) ? 1 : 0;
            }
          else           /* currently dead */
            {
              new[y][x] = (n == 3) ? 1 : 0;
            }
        }
    }
}

static
inline
void
show_visible_grid (int **g)
{
  int y, x;

  for (y = 1; y < grid_y - 1; y += 1)
    {
      for (x = 1; x < grid_x - 1; x += 1)
        {
          printf ("%c", g[y][x] ? live_cell : dead_cell);
        }
      printf ("\n");
    }
  printf ("\n");
}

static
inline
void
swap_grids (int ***from, int ***to)
{
  int **swap = *from;
  *from = *to;
  *to = swap;
}

static
inline
void
help_message (int opt)
{
  fprintf (stderr,
           "Usage: %s [options]\n",
           progname);
  fprintf (stderr, "\n");
  fprintf (stderr,
           "    -s micro-seconds sleep between updates (default %d)\n",
           default_sleep_update_us);
  fprintf (stderr,
           "    -n iterations    update how many times (default forever)\n");
           fprintf (stderr,
           "    -h               this help message\n");
  fprintf (stderr,
           "    -y y_size        height of grid (default %d)\n",
           default_visible_y);
  fprintf (stderr,
           "    -x x_size        width of grid (default %d)\n",
           default_visible_x);
  fprintf (stderr,
           "    -a alive-char    alive cells show this (default '%c')\n",
           default_live_cell);
  fprintf (stderr,
           "    -d dead-char     dead cells show this (default '%c')\n",
           default_dead_cell);
  fprintf (stderr, "\n");
}

int
main (int argc, char *argv[])
{
  int opt;
  unsigned long iter;

  int **old_grid;
  int **new_grid; /* current and new grid generation */

  int **from;
  int **to; /* double-buffer */

  progname = basename (argv[0]);

  sleep_update_us = default_sleep_update_us;
  iterations = default_iterations;
  visible_y = default_visible_y;
  visible_x = default_visible_x;
  live_cell = default_live_cell;
  dead_cell = default_dead_cell;

  while ((opt = getopt (argc, argv, "s:n:y:x:ha:d:")) != -1)
    {
      switch (opt) {
      case 's':
        sleep_update_us = atoi (optarg);
        break;
      case 'n':
        iterations = atoi (optarg);
        break;
      case 'y':
        visible_y = atoi (optarg);
        break;
      case 'x':
        visible_x = atoi (optarg);
        break;
      case 'a':
        live_cell = *optarg;
        break;
      case 'd':
        dead_cell = *optarg;
        break;
      default: /* '?' */
        help_message (opt);
        exit (EXIT_FAILURE);
        break;
      }
    }

  /**
   * full grid has a dead border
   */
  grid_y = visible_y + 2;
  grid_x = visible_x + 2;

  old_grid = allocate_grid();
  new_grid = allocate_grid();

  from = old_grid;
  to = new_grid;

  zero_grid (from);

  /**
   * allow user to choose how to initialize in future
   */
  randomize_visible_grid (from);
  // beacon_grid (from);
  // load_visible_grid ("glider.life", from);

  for (iter = 0; iter != iterations; iter += 1)
    {
      system ("clear");  /* should we use curses or similar here? */

      show_visible_grid (from);

      usleep (sleep_update_us);

      update_visible_grid (from, to);

      swap_grids (&from, &to);
    }

  return 0;
}
