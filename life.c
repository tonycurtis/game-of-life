#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define Y 60
#define X 140

static int grid_y;
static int grid_x;

static
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

static
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

static
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

static
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

int
main (int argc, char *argv[])
{
  int **old_grid;
  int **new_grid;
  int **from;
  int **to;

  useconds_t delay = 1e5;

  grid_y = Y + 2;
  grid_x = X + 2;

  old_grid = allocate_grid();
  from = old_grid;

  new_grid = allocate_grid();
  to = new_grid;

  zero_grid (from);

  // randomize_visible_grid (from);
  // beacon_grid (from);
  load_visible_grid ("glider.life", from);

  while (true)
    {
      system ("clear");

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
