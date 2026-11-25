#include "ciutil.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
  /* text mode would turn "\n" into "\r\n" and corrupt raw b64dec output */
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stderr), _O_BINARY);
#endif
  return ciutil_cli_run(argc, argv, stdout, stderr);
}
