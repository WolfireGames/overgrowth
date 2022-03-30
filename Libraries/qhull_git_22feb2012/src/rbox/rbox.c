/*<html><pre>  -<a                             href="../libqhull/index.htm#TOC"
  >-------------------------------</a><a name="TOP">-</a>

   rbox.c
     rbox program for generating input points for qhull.

   notes:
     50 points generated for 'rbox D4'

*/

#include "random.h"
#include "libqhull.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if __MWERKS__ && __POWERPC__
#include <SIOUX.h>
#include <Files.h>
#include <console.h>
#include <Desk.h>
#endif

#ifdef _MSC_VER  /* Microsoft Visual C++ -- warning level 4 */
#pragma warning( disable : 4706)  /* assignment within conditional function */
#endif

char prompt[]= "\n\
-rbox- generate various point distributions.  Default is random in cube.\n\
\n\
args (any order, space separated):                    Version: 2001/06/24\n\
  3000    number of random points in cube, lens, spiral, sphere or grid\n\
  D3      dimension 3-d\n\
  c       add a unit cube to the output ('c G2.0' sets size)\n\
  d       add a unit diamond to the output ('d G2.0' sets size)\n\
  l       generate a regular 3-d spiral\n\
  r       generate a regular polygon, ('r s Z1 G0.1' makes a cone)\n\
  s       generate cospherical points\n\
  x       generate random points in simplex, may use 'r' or 'Wn'\n\
  y       same as 'x', plus simplex\n\
  Pn,m,r  add point [n,m,r] first, pads with 0\n\
\n\
  Ln      lens distribution of radius n.  Also 's', 'r', 'G', 'W'.\n\
  Mn,m,r  lattice(Mesh) rotated by [n,-m,0], [m,n,0], [0,0,r], ...\n\
          '27 M1,0,1' is {0,1,2} x {0,1,2} x {0,1,2}.  Try 'M3,4 z'.\n\
  W0.1    random distribution within 0.1 of the cube's or sphere's surface\n\
  Z0.5 s  random points in a 0.5 disk projected to a sphere\n\
  Z0.5 s G0.6 same as Z0.5 within a 0.6 gap\n\
\n\
  Bn      bounding box coordinates, default %2.2g\n\
  h       output as homogeneous coordinates for cdd\n\
  n       remove command line from the first line of output\n\
  On      offset coordinates by n\n\
  t       use time as the random number seed(default is command line)\n\
  tn      use n as the random number seed\n\
  z       print integer coordinates, default 'Bn' is %2.2g\n\
";

/*--------------------------------------------
-rbox-  main procedure of rbox application
*/
int main(int argc, char **argv) {
  char *command;
  int command_size;
  int return_status;

#if __MWERKS__ && __POWERPC__
  char inBuf[BUFSIZ], outBuf[BUFSIZ], errBuf[BUFSIZ];
  SIOUXSettings.showstatusline= False;
  SIOUXSettings.tabspaces= 1;
  SIOUXSettings.rows= 40;
  if (setvbuf(stdin, inBuf, _IOFBF, sizeof(inBuf)) < 0   /* w/o, SIOUX I/O is slow*/
  || setvbuf(stdout, outBuf, _IOFBF, sizeof(outBuf)) < 0
  || (stdout != stderr && setvbuf(stderr, errBuf, _IOFBF, sizeof(errBuf)) < 0))
    fprintf(stderr, "qhull internal warning (main): could not change stdio to fully buffered.\n");
  argc= ccommand(&argv);
#endif

  if (argc == 1) {
    printf(prompt, qh_DEFAULTbox, qh_DEFAULTzbox);
    return 1;
  }

  command_size= qh_argv_to_command_size(argc, argv);
  if ((command= (char *)qh_malloc((size_t)command_size))) {
    if (!qh_argv_to_command(argc, argv, command, command_size)) {
      fprintf(stderr, "rbox internal error: allocated insufficient memory (%d) for arguments\n", command_size);
      return_status= qh_ERRinput;
    }else{
      return_status= qh_rboxpoints(stdout, stderr, command);
    }
    qh_free(command);
  }else {
    fprintf(stderr, "rbox error: insufficient memory for %d bytes\n", command_size);
    return_status= qh_ERRmem;
  }
  return return_status;
}/*main*/

