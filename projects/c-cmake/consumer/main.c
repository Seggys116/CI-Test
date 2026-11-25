#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <ciutil.h>

int main(void) {
  const char *text = "123456789";
  printf("%08" PRIx32 "\n", ciutil_crc32(text, strlen(text)));
  return 0;
}
