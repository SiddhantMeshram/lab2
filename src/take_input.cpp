#include <iostream>
#include <string>
#include <unistd.h>

#include "main.h"

using namespace std;

int main(int argc, char *argv[]) {

  bool verbose, preemption_logs;
  string scheduler_type;
  int c;
  while((c = getopt(argc, argv, "vps:")) != -1)
    switch (c) {
      case 'v':
        verbose = true;
        break;
      case 'p':
        preemption_logs = true;
        break;
      case 's':
        scheduler_type = optarg;
        break;
      case '?':
        if (optopt == 's')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
    }

  Main m(scheduler_type, argv[optind], argv[optind + 1], verbose,
         preemption_logs);
}