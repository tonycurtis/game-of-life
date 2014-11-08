#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <libgen.h>

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

	  if (old[y][x])	/* currently alive */
	    {
	      new[y][x] = ((n == 2) || (n == 3)) ? 1 : 0;
	    }
	  else			/* currently dead */
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
      printf ("    ");
      for (x = 1; x < grid_x - 1; x += 1)
	{
	  printf ("%c", g[y][x] ? '*' : ' ');
	}
      printf ("\n");
    }
  printf ("\n");
}

static
inline
void
help_message (int opt)
{
  fprintf (stderr,
	   "Usage: %s [-h] [-y y_size] [-x x_size] [-d micro-seconds-delay]\n",
	   progname);
}

int
main (int argc, char *argv[])
{
  int **old_grid;
  int **new_grid; /* current and new grid generation */

  int **from;
  int **to; /* double-buffer */

  useconds_t delay = 1e5;  /* micro-seconds wait between screen updates */
  int visible_y = 60;
  int visible_x = 140;
  int opt;

  progname = basename (argv[0]);

  while ((opt = getopt (argc, argv, "d:y:x:h")) != -1)
    {
      switch (opt) {
      case 'd':
	delay = atoi (optarg);
	break;
      case 'y':
	visible_y = atoi (optarg);
	break;
      case 'x':
	visible_x = atoi (optarg);
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

  while (true)
    {
      system ("clear");  /* should we use curses or similar here? */

      show_visible_grid (from);

      usleep (delay);

      update_visible_grid (from, to);

      {
	int **swap = from;
	from = to;
	to = swap;
      }

    }

  return 0;
}
