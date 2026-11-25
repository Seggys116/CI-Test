#include "ciutil.h"

#include <stdlib.h>
#include <string.h>

static int is_digit(char c) {
  return c >= '0' && c <= '9';
}

static int is_ident_char(char c) {
  return is_digit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
}

static void free_list(char **items, size_t count) {
  size_t i;
  for (i = 0; i < count; i++) {
    free(items[i]);
  }
  free(items);
}

static char *dup_range(const char *s, size_t len) {
  char *copy = (char *)malloc(len + 1u);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, s, len);
  copy[len] = '\0';
  return copy;
}

static ciutil_status parse_number(const char **cursor, uint64_t *out) {
  const char *p = *cursor;
  uint64_t v = 0;

  if (!is_digit(*p)) {
    return CIUTIL_ERR_SEMVER_SYNTAX;
  }
  if (*p == '0' && is_digit(p[1])) {
    return CIUTIL_ERR_SEMVER_SYNTAX;
  }
  while (is_digit(*p)) {
    uint64_t d = (uint64_t)(*p - '0');
    if (v > (UINT64_MAX - d) / 10u) {
      return CIUTIL_ERR_SEMVER_OVERFLOW;
    }
    v = v * 10u + d;
    p++;
  }
  *out = v;
  *cursor = p;
  return CIUTIL_OK;
}

/* splits a dot separated identifier list; prerelease numeric identifiers may not have leading zeros */
static ciutil_status parse_idents(const char *s, size_t len, int prerelease, char ***list,
                                  size_t *count) {
  size_t n = 1;
  size_t run = 0;
  size_t i;
  size_t start = 0;
  size_t idx = 0;
  int all_digits = 1;
  char **items;

  for (i = 0; i <= len; i++) {
    if (i == len || s[i] == '.') {
      if (run == 0) {
        return CIUTIL_ERR_SEMVER_SYNTAX;
      }
      if (prerelease && all_digits && run > 1u && s[i - run] == '0') {
        return CIUTIL_ERR_SEMVER_SYNTAX;
      }
      if (i < len) {
        n++;
      }
      run = 0;
      all_digits = 1;
    } else {
      if (!is_ident_char(s[i])) {
        return CIUTIL_ERR_SEMVER_SYNTAX;
      }
      if (!is_digit(s[i])) {
        all_digits = 0;
      }
      run++;
    }
  }

  items = (char **)calloc(n, sizeof *items);
  if (items == NULL) {
    return CIUTIL_ERR_NO_MEMORY;
  }
  for (i = 0; i <= len; i++) {
    if (i == len || s[i] == '.') {
      items[idx] = dup_range(s + start, i - start);
      if (items[idx] == NULL) {
        free_list(items, idx);
        return CIUTIL_ERR_NO_MEMORY;
      }
      idx++;
      start = i + 1u;
    }
  }
  *list = items;
  *count = n;
  return CIUTIL_OK;
}

ciutil_status ciutil_semver_parse(const char *text, ciutil_semver *out) {
  ciutil_semver v;
  const char *p;
  const char *end;
  ciutil_status status;

  if (text == NULL || out == NULL) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  memset(out, 0, sizeof *out);
  memset(&v, 0, sizeof v);

  p = text;
  status = parse_number(&p, &v.major);
  if (status != CIUTIL_OK) {
    return status;
  }
  if (*p != '.') {
    return CIUTIL_ERR_SEMVER_SYNTAX;
  }
  p++;
  status = parse_number(&p, &v.minor);
  if (status != CIUTIL_OK) {
    return status;
  }
  if (*p != '.') {
    return CIUTIL_ERR_SEMVER_SYNTAX;
  }
  p++;
  status = parse_number(&p, &v.patch);
  if (status != CIUTIL_OK) {
    return status;
  }

  if (*p == '-') {
    p++;
    end = p;
    while (*end != '\0' && *end != '+') {
      end++;
    }
    status = parse_idents(p, (size_t)(end - p), 1, &v.prerelease, &v.prerelease_count);
    if (status != CIUTIL_OK) {
      return status;
    }
    p = end;
  }
  if (*p == '+') {
    p++;
    end = p + strlen(p);
    status = parse_idents(p, (size_t)(end - p), 0, &v.build, &v.build_count);
    if (status != CIUTIL_OK) {
      free_list(v.prerelease, v.prerelease_count);
      return status;
    }
    p = end;
  }
  if (*p != '\0') {
    free_list(v.prerelease, v.prerelease_count);
    free_list(v.build, v.build_count);
    return CIUTIL_ERR_SEMVER_SYNTAX;
  }

  *out = v;
  return CIUTIL_OK;
}

void ciutil_semver_free(ciutil_semver *version) {
  if (version == NULL) {
    return;
  }
  free_list(version->prerelease, version->prerelease_count);
  free_list(version->build, version->build_count);
  memset(version, 0, sizeof *version);
}

static int compare_u64(uint64_t a, uint64_t b) {
  return a < b ? -1 : (a > b ? 1 : 0);
}

static int is_numeric(const char *s) {
  for (; *s != '\0'; s++) {
    if (!is_digit(*s)) {
      return 0;
    }
  }
  return 1;
}

static int compare_ident(const char *a, const char *b) {
  int a_num = is_numeric(a);
  int b_num = is_numeric(b);
  int c;

  if (a_num && b_num) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    if (la != lb) {
      return la < lb ? -1 : 1;
    }
  } else if (a_num) {
    return -1;
  } else if (b_num) {
    return 1;
  }
  c = strcmp(a, b);
  return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

int ciutil_semver_compare(const ciutil_semver *a, const ciutil_semver *b) {
  size_t shared;
  size_t i;
  int c;

  c = compare_u64(a->major, b->major);
  if (c != 0) {
    return c;
  }
  c = compare_u64(a->minor, b->minor);
  if (c != 0) {
    return c;
  }
  c = compare_u64(a->patch, b->patch);
  if (c != 0) {
    return c;
  }

  if (a->prerelease_count == 0 || b->prerelease_count == 0) {
    if (a->prerelease_count == b->prerelease_count) {
      return 0;
    }
    return a->prerelease_count == 0 ? 1 : -1;
  }

  shared = a->prerelease_count < b->prerelease_count ? a->prerelease_count : b->prerelease_count;
  for (i = 0; i < shared; i++) {
    c = compare_ident(a->prerelease[i], b->prerelease[i]);
    if (c != 0) {
      return c;
    }
  }
  if (a->prerelease_count == b->prerelease_count) {
    return 0;
  }
  return a->prerelease_count < b->prerelease_count ? -1 : 1;
}

static size_t dec_len(uint64_t v) {
  size_t n = 1;
  while (v >= 10u) {
    v /= 10u;
    n++;
  }
  return n;
}

static size_t put_dec(char *dst, uint64_t v) {
  size_t n = dec_len(v);
  size_t i;
  for (i = n; i > 0; i--) {
    dst[i - 1u] = (char)('0' + (int)(v % 10u));
    v /= 10u;
  }
  return n;
}

static size_t idents_len(char *const *items, size_t count) {
  size_t total = 0;
  size_t i;
  for (i = 0; i < count; i++) {
    total += strlen(items[i]);
  }
  return count > 0 ? total + count - 1u : 0;
}

static size_t put_idents(char *dst, char *const *items, size_t count) {
  size_t o = 0;
  size_t i;
  for (i = 0; i < count; i++) {
    size_t n = strlen(items[i]);
    if (i > 0) {
      dst[o++] = '.';
    }
    memcpy(dst + o, items[i], n);
    o += n;
  }
  return o;
}

ciutil_status ciutil_semver_to_string(const ciutil_semver *version, char **out) {
  size_t total;
  size_t o = 0;
  char *buf;

  if (version == NULL || out == NULL) {
    return CIUTIL_ERR_INVALID_ARG;
  }
  *out = NULL;

  total = dec_len(version->major) + dec_len(version->minor) + dec_len(version->patch) + 2u;
  if (version->prerelease_count > 0) {
    total += 1u + idents_len(version->prerelease, version->prerelease_count);
  }
  if (version->build_count > 0) {
    total += 1u + idents_len(version->build, version->build_count);
  }

  buf = (char *)malloc(total + 1u);
  if (buf == NULL) {
    return CIUTIL_ERR_NO_MEMORY;
  }
  o += put_dec(buf + o, version->major);
  buf[o++] = '.';
  o += put_dec(buf + o, version->minor);
  buf[o++] = '.';
  o += put_dec(buf + o, version->patch);
  if (version->prerelease_count > 0) {
    buf[o++] = '-';
    o += put_idents(buf + o, version->prerelease, version->prerelease_count);
  }
  if (version->build_count > 0) {
    buf[o++] = '+';
    o += put_idents(buf + o, version->build, version->build_count);
  }
  buf[o] = '\0';
  *out = buf;
  return CIUTIL_OK;
}
