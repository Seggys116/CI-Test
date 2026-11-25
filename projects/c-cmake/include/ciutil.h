#ifndef CIUTIL_H
#define CIUTIL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define CIUTIL_VERSION "1.0.0"

/* longest canonical numeral is MMMDCCCLXXXVIII (3888), 15 characters */
#define CIUTIL_ROMAN_MAX_LEN 15

/* CIUTIL_SHARED is defined for shared builds (publicly, so consumers see it);
   CIUTIL_BUILDING is defined only while compiling the library itself */
#if defined(CIUTIL_SHARED) && defined(_WIN32)
#ifdef CIUTIL_BUILDING
#define CIUTIL_API __declspec(dllexport)
#else
#define CIUTIL_API __declspec(dllimport)
#endif
#elif defined(CIUTIL_SHARED) && defined(__GNUC__)
#define CIUTIL_API __attribute__((visibility("default")))
#else
#define CIUTIL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ciutil_status {
  CIUTIL_OK = 0,
  CIUTIL_ERR_INVALID_ARG,
  CIUTIL_ERR_NO_MEMORY,
  CIUTIL_ERR_BUFFER_TOO_SMALL,
  CIUTIL_ERR_BASE64_LENGTH,
  CIUTIL_ERR_BASE64_CHAR,
  CIUTIL_ERR_BASE64_PADDING,
  CIUTIL_ERR_BASE64_TRAILING_BITS,
  CIUTIL_ERR_ROMAN_RANGE,
  CIUTIL_ERR_ROMAN_SYNTAX,
  CIUTIL_ERR_SEMVER_SYNTAX,
  CIUTIL_ERR_SEMVER_OVERFLOW
} ciutil_status;

CIUTIL_API const char *ciutil_status_message(ciutil_status status);

/* releases memory returned by any ciutil function documented as malloc'd */
CIUTIL_API void ciutil_free(void *ptr);

/* crc-32/ieee. incremental use: state = init(); state = update(state, ...); crc = final(state) */
CIUTIL_API uint32_t ciutil_crc32(const void *data, size_t len);
CIUTIL_API uint32_t ciutil_crc32_init(void);
CIUTIL_API uint32_t ciutil_crc32_update(uint32_t state, const void *data, size_t len);
CIUTIL_API uint32_t ciutil_crc32_final(uint32_t state);

/* adler-32. the running state is the checksum itself: start from init(), feed with update() */
CIUTIL_API uint32_t ciutil_adler32(const void *data, size_t len);
CIUTIL_API uint32_t ciutil_adler32_init(void);
CIUTIL_API uint32_t ciutil_adler32_update(uint32_t state, const void *data, size_t len);

/* base64 (rfc 4648, padded, strict). encoded_len excludes the terminating nul and
   returns SIZE_MAX when the input is too large to encode. */
CIUTIL_API size_t ciutil_base64_encoded_len(size_t len);

/* writes encoded_len(len) characters plus a nul, so cap must be at least encoded_len(len) + 1 */
CIUTIL_API ciutil_status ciutil_base64_encode_buf(const void *data, size_t len, char *out,
                                                  size_t cap, size_t *written);

/* malloc'd nul-terminated result, release with ciutil_free */
CIUTIL_API ciutil_status ciutil_base64_encode(const void *data, size_t len, char **out);

/* text is not required to be nul-terminated; embedded nul bytes are rejected as invalid characters */
CIUTIL_API ciutil_status ciutil_base64_decode(const char *text, size_t len, uint8_t *out,
                                              size_t cap, size_t *written);

/* malloc'd result (never NULL on success, even for empty output), release with ciutil_free */
CIUTIL_API ciutil_status ciutil_base64_decode_alloc(const char *text, size_t len, uint8_t **out,
                                                    size_t *out_len);

/* roman numerals, 1..3999. cap must be at least CIUTIL_ROMAN_MAX_LEN + 1; result is nul-terminated */
CIUTIL_API ciutil_status ciutil_roman_encode(int value, char *out, size_t cap, size_t *written);

/* strict: only the canonical spelling of each value is accepted */
CIUTIL_API ciutil_status ciutil_roman_decode(const char *text, unsigned int *value);

/* semver 2.0.0 */
typedef struct ciutil_semver {
  uint64_t major;
  uint64_t minor;
  uint64_t patch;
  char **prerelease;
  size_t prerelease_count;
  char **build;
  size_t build_count;
} ciutil_semver;

/* on failure *out is zeroed and need not be freed */
CIUTIL_API ciutil_status ciutil_semver_parse(const char *text, ciutil_semver *out);
CIUTIL_API void ciutil_semver_free(ciutil_semver *version);

/* returns -1, 0 or 1 using semver precedence; build metadata is ignored */
CIUTIL_API int ciutil_semver_compare(const ciutil_semver *a, const ciutil_semver *b);

/* malloc'd string, release with ciutil_free */
CIUTIL_API ciutil_status ciutil_semver_to_string(const ciutil_semver *version, char **out);

/* command line driver. argv[0] is the program name, as in main. returns the process exit code:
   0 success, 2 runtime error, 64 usage error */
CIUTIL_API int ciutil_cli_run(int argc, char *const *argv, FILE *out, FILE *err);

#ifdef __cplusplus
}
#endif

#endif
