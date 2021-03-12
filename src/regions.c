/*------------------------------------------------------------
 * regions.c
 * 
 * This file is part of REGIONS
 *
 * Copyright (c) 2019 - 2021 Matthew Love <matthew.love@colorado.edu>
 * BOUNDS is liscensed under the GPL v.2 or later and 
 * is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 * <http://www.gnu.org/licenses/> 
 *--------------------------------------------------------------*/

#include <stdio.h> 
#include <stdlib.h> 
#include <getopt.h>
#include <string.h>
#include <stddef.h>

#define REGIONS_VERSION "0.0.4"

typedef struct 
{
  double xmin;
  double xmax;
  double ymin;
  double ymax;
} region_t;

typedef struct 
{
  double x;
  double y;
} point_t;

typedef region_t* region_ptr_t;

/* Flags set by `--version' and `--help' */
static int version_flag;
static int help_flag;
static int verbose_flag;

static void
print_version(const char* command_name, const char* command_version) 
{
  fprintf(stderr, "%s %s \n\
Copyright © 2019 - 2021 Matthew Love <matthew.love@colorado.edu> \n\
%s is liscensed under the GPL v.2 or later and is \n\
distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;\n\
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\
<http://www.gnu.org/licenses/>\n", command_name, command_version, command_name);
  exit (1);
}

static void 
usage() {
  fprintf(stderr, "\
regions [OPTION]... -R<region> -R<region>... \n\
Manipulate the given region(s) where a REGION is a rectangle which represents\n\
a specific geographic location.\n\
\n\
  -R, --region\t\tthe input region <west/east/south/north>\n\
  -b, --buffer\t\tbuffer the region(s) by value\n\
  -m, --merge\t\tmerge the input region(s)\n\
  -e, --echo\t\techo the (processed) region(s)\n\
  -n, --name\t\techo the (processed) region(s) as a name-string\n\n\
      --verbose\t\tincrease the verbosity.\n\
      --help\t\tprint this help menu and exit.\n\
      --version\t\tprint version information and exit.\n\
\n\
CIRES DEM home page: <http://ciresgroups.colorado.edu/coastalDEM>\n\
");
  exit (1);
}

int
region_p (region_t *region) 
{
  if (region->xmin >= region->xmax) return 0;
  if (region->ymin >= region->ymax) return 0;
  return 1;
}

void
region_echo (region_t *region, int eflag) 
{
  if (eflag == 2) printf("%f/%f/%f/%f\n", region->xmin, region->xmax, region->ymin, region->ymax);
  else printf("-R%f/%f/%f/%f\n", region->xmin, region->xmax, region->ymin, region->ymax);
}

void
region_name (region_t *region) 
{
  char* ns;
  char* ew;
  if (region->ymax < 0) ns = "s";
  else ns = "n";
  if (region->xmin < 0) ew = "w";
  else ew = "s";
  printf("%s%02dx%02d_%s%03dx%02d\n",
	 ns, abs((int)region->ymax), abs(((int)(region->ymax*100)%100)),
	 ew, abs((int)region->xmin), abs((int)(region->xmin*100)%100));
}

void
region_format (region_t *region, int aflag) 
{
  /* This is for the GMT compatibility. More can be done here.
   */
  if (aflag == 0)
    printf("# @VGMT1.0 @GMULTIPOLYGON\n# @NName\n# @Tstring\n# FEATURE_DATA\n");
  printf(">\n# @D%s\n# @P\n", "regions");
  printf("%f %f\n", region->xmin, region->ymax);
  printf("%f %f\n", region->xmax, region->ymax);
  printf("%f %f\n", region->xmax, region->ymin);
  printf("%f %f\n", region->xmin, region->ymin);
  printf("%f %f\n", region->xmin, region->ymax);
}

void
region_merge (region_t *regions, int rsize) 
{
  int i;
  for (i=0; i < rsize; i++) 
    if (i > 0)
      {
	if (regions[i].xmin < regions[0].xmin) 
	  regions[0].xmin = regions[i].xmin;

	if (regions[i].xmax > regions[0].xmax)
	  regions[0].xmax = regions[i].xmax;

	if (regions[i].ymin < regions[0].ymin) 
	  regions[0].ymin = regions[i].ymin;

	if (regions[i].ymax > regions[0].ymax)
	  regions[0].ymax = regions[i].ymax;
      }
}

void
region_extend (region_t *region, double xval) 
{
  region->xmin = region->xmin - xval;
  region->xmax = region->xmax + xval;
  region->ymin = region->ymin - xval;
  region->ymax = region->ymax + xval;
}

point_t
region_center (region_t *region)
{
  point_t pnt;
  pnt.x = region->xmin + ((region->xmax - region->xmin) / 2);
  pnt.y = region->ymin + ((region->ymax - region->ymin) / 2);
  return pnt;
}

region_t
region_parse (char* region_string) 
{

  int j;
  region_t rgn;

  char* p = strtok (region_string, "/");
  for (j = 0; j < 4; j++) {
    if (p != NULL) {
      if (j==0) rgn.xmin = atof (p);
      if (j==1) rgn.xmax = atof (p);
      if (j==2) rgn.ymin = atof (p);
      if (j==3) rgn.ymax = atof (p);
    }
    p = strtok (NULL, "/");
  }
  return rgn;
}
  
int
main (int argc, char **argv) 
{

  int c, status;
  int rflag = 0, mflag = 0, eflag = 0, nflag = 0, bflag = 0;
  char* region;
  char* tmp_region;
  double bval;
  int pf_length;
  ssize_t n = 0;
  point_t pnt;
  
  char REGION_D[] = "-180/180/-90/90";
  char REGION_G[] = "0/360/-90/90";

  region_t* rgns;
  rgns = (region_t*) malloc (sizeof (region_t));

  while (1) 
    {
      static struct option long_options[] =
	{
	  /* These options set a flag. */
	  {"version", no_argument, &version_flag, 1},
	  {"help", no_argument, &help_flag, 1},
	  {"verbose", no_argument, &verbose_flag, 1},
	  /* These options don't set a flag.
	     We distinguish them by their indices. */
	  {"region", required_argument, 0, 'R'},
	  {"merge", no_argument, 0, 'm'},
	  {"buffer", required_argument, 0, 'b'},
	  {"echo", no_argument, 0, 'e'},
	  {"name", no_argument, 0, 'n'},
	  {0, 0, 0, 0}
	};
      /* getopt_long stores the option index here. */
      int option_index = 0;
      
      c = getopt_long (argc, argv, "R:menb:",
		       long_options, &option_index);
      
      /* Detect the end of the options. */
      if (c == -1) break;
      
      switch (c) 
	{
	case 0:
	  /* If this option set a flag, do nothing else now. */
	  if (long_options[option_index].flag != 0)
	    break;
	  printf ("option %s", long_options[option_index].name);
	  if (optarg)
	    printf (" with arg %s", optarg);
	  printf ("\n");
	  break;
	case 'R':
	  n++;
	  
	  tmp_region = optarg;
	  
	  if (strcmp (tmp_region, "d") == 0) region = REGION_D;
	  else if (strcmp (tmp_region, "g") == 0) region = REGION_G;
	  else region = tmp_region;
	  
	  region_t* temp = realloc (rgns, n * sizeof (region_t));
	  rgns = temp;
	  
	  rgns[rflag] = region_parse (region);
	  rflag++;
	  break;
	case 'm':
	  mflag++;
	  break;
	case 'e':
	  eflag++;
	  break;
	case 'n':
	  nflag++;
	  break;
	case 'b':
	  bflag++;
	  bval = atof (optarg);
	  break;
	  
	case '?':
	  /* getopt_long already printed an error message. */
	  fprintf(stderr,"Try 'regions --help' for more information.\n");
	  exit(0);
	  break;
	  
	default:
	  abort ();
	}
    }

  if (version_flag) print_version ("regions", REGIONS_VERSION);
  if (help_flag) usage();

  if (!rflag) usage();
  
  int i, a;

  // merge regions
  if (mflag) {
    region_merge (rgns,rflag);
    rflag = 1;
  }

  // parse through regions and print them to stdout.
  for (i=0; i<rflag; i++) 
    if (region_p(&rgns[i])) 
      {
	if (bflag) 
	  region_extend (&rgns[i], bval);
	if (eflag) 
	  region_echo (&rgns[i], eflag);
	else if (nflag) 
	  region_name (&rgns[i]);
	else 
	  region_format (&rgns[i], i);
      }

  free (rgns);
  rgns = NULL;
  exit (0);
}
