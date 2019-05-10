// Do not modify this file. It will be replaced by the grading scripts
// when checking your project.

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  printf(1, "%s", "** Placeholder program for grading scripts **\n");

  int i = getopenedcount();
  printf(1, "%s", "Initial openedcount is: "); 
  printf(1, "%d\n", i); 
 

  open(".", O_CREATE );
  open(".", O_CREATE );
  open(".", O_CREATE );
  open(".", O_CREATE );
  open(".", O_CREATE );

  printf(1, "%s", "After 5 loops the opennedcount is:");
  int j = getopenedcount();
  printf(1, "%d\n", j);
 
 
 
  exit();
}
