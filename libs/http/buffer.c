//
// buffer.c
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include "buffer.h"
#include "dlog.h"

// TODO: shared with reference counting
// TODO: linked list for append/prepend etc

/*
 * Compute the nearest multiple of `a` from `b`.
 */
#define nearest_multiple_of(a, b)   (((b) + ((a) - 1)) & ~((a) - 1))

int IsPowerOf2(size_t size);
size_t RoundupPowOf2(size_t size);

/*
 * Allocate a new buffer with BUFFER_DEFAULT_SIZE.
 */

buffer_t *
buffer_new() {
  return buffer_new_with_size(BUFFER_DEFAULT_SIZE);
}

/*
 * Allocate a new buffer with `n` bytes.
 */

buffer_t *
buffer_new_with_size(size_t size) {
  buffer_t *self = malloc(sizeof(buffer_t));
  if (!self) return NULL;
  size = RoundupPowOf2(size);
  self->len = 0;
  self->size = size;
  self->data = self->alloc = calloc(size + 1, 1);
  return self;
}

/*
 * Allocate a new buffer with `str`.
 */

buffer_t *
buffer_new_with_string(char *str) {
  return buffer_new_with_string_length(str, strlen(str));
}

/*
 * Allocate a new buffer with `str` and `len`.
 */

buffer_t *
buffer_new_with_string_length(char *str, size_t len) {
  buffer_t *self = buffer_new_with_size(len);
  if (!self) return NULL;
  memcpy(self->alloc, str, len);
  self->data = self->alloc;
  self->len = len;
  return self;
}

/*
 * Deallocate excess memory, the number
 * of bytes removed or -1.
 */

ssize_t
buffer_compact(buffer_t *self) {
  size_t len = self->len;
  size_t rem = self->size - self->len;
  char *buf = calloc(len + 1, 1);
  if (!buf) return -1;
  memcpy(buf, self->data, len);
  free(self->alloc);
  self->size = len;
  self->data = self->alloc = buf;
  return rem;
}

/*
 * Free the buffer.
 */

void
buffer_free(buffer_t *self) {
  free(self->alloc);
  free(self);
}

/*
 * Return buffer size.
 */

size_t
buffer_size(buffer_t *self) {
  return self->size;
}

/*
 * Return string length.
 */

size_t
buffer_length(buffer_t *self) {
  return self->len;
}

/*
 * Resize to hold `n` bytes.
 */

int
buffer_resize(buffer_t *self, size_t n) {
  //n = nearest_multiple_of(1024, n);
  n = RoundupPowOf2(n);
  self->size = n;
  self->alloc = self->data = realloc(self->alloc, n + 1);
  if (!self->alloc) return -1;
  self->alloc[n] = '\0';
  return 0;
}

/*
 * Append a printf-style formatted string to the buffer.
 */

int buffer_appendf(buffer_t *self, const char *format, ...) {
  va_list ap;
  va_list tmpa;
  char *dst = NULL;
  int required = 0;
  int bytes = 0;

  // First, we compute how many bytes are needed
  // for the formatted string and allocate that
  // much more space in the buffer.
  va_start(ap, format);
  va_copy(tmpa, ap);
  required = vsnprintf(NULL, 0, format, tmpa);
  va_end(tmpa);

  if(self->len + required >= self->size){
    if (-1 == buffer_resize(self, self->size + required)) {
      va_end(ap);
      return -1;
    }
  }

  // Next format the string into the space that we
  // have made room for.
  dst = self->data + self->len;
  bytes = vsnprintf(dst, 1 + required, format, ap);
  self->len += required;
  va_end(ap);

  return bytes < 0 ? -1 : 0;
}

/*
 * Append `str` to `self` and return 0 on success, -1 on failure.
 */

int
buffer_append(buffer_t *self, const char *str) {
  return buffer_append_n(self, str, strlen(str));
}

/*
 * Append the first `len` bytes from `str` to `self` and
 * return 0 on success, -1 on failure.
 */
int
buffer_append_n(buffer_t *self, const char *str, size_t len) {
  size_t prev = self->len;
  size_t needed = len + prev;

  // enough space
  if (self->size > needed) {
    strncat(self->data, str, len);
    self->len += len;
    return 0;
  }

  // resize
  int ret = buffer_resize(self, self->size + len);
  if (-1 == ret) return -1;
  strncat(self->data, str, len);
  self->len += len;

  return 0;
}

/*
 * Prepend `str` to `self` and return 0 on success, -1 on failure.
 * 将str插入到self前，即前追加模式
 */

int
buffer_prepend(buffer_t *self, char *str) {
  size_t len = strlen(str);
  size_t prev = self->len;
  size_t needed = len + prev;

  // enough space
  if (self->len > needed) goto move;

  // resize
  int ret = buffer_resize(self, needed);
  if (-1 == ret) return -1;

  // move
  move:
  memmove(self->data + len, self->data, len + 1);
  memcpy(self->data, str, len);
  self->len += len;

  return 0;
}

/*
 * Return a new buffer based on the `from..to` slice of `buf`,
 * or NULL on error.
 */

buffer_t *
buffer_slice(buffer_t *buf, size_t from, ssize_t to) {
  size_t len = buf->len;

  // bad range
  if (to < from) return NULL;

  // relative to end
  if (to < 0) to = len - ~to;

  // cap end
  if (to > len) to = len;

  size_t n = to - from;
  buffer_t *self = buffer_new_with_size(n);
  memcpy(self->data, buf->data + from, n);
  self->len = n;

  return self;
}

/*
 * Return 1 if the buffers contain equivalent data.
 */

int
buffer_equals(buffer_t *self, buffer_t *other) {
  return (self->len == other->len && 0 == strcmp(self->data, other->data));
}

/*
 * Return the index of the substring `str`, or -1 on failure.
 */

ssize_t
buffer_indexof(buffer_t *self, char *str) {
  char *sub = strstr(self->data, str);
  if (!sub) return -1;
  return sub - self->data;
}

/*
 * Trim leading whitespace.
 */

void
buffer_trim_left(buffer_t *self) {
  int c = 0, ncut = 0;
  char* ptmp = self->data;
  while ((c = *ptmp) && isspace(c)) {
    ++ptmp;
  }
  ncut = ptmp - self->data;
  if(ncut > 0) {
    self->len -= ncut;
    memmove(self->data, ptmp, self->len);
  }
}

/*
 * Trim trailing whitespace.
 */

void
buffer_trim_right(buffer_t *self) {
  int c;
  size_t i = self->len - 1;
  while((c = self->data[i]) && isspace(c)) {
    self->data[i--] = 0;
  }
  self->len = i + 1;
}

/*
 * Trim trailing and leading whitespace.
 */

void
buffer_trim(buffer_t *self) {
  buffer_trim_left(self);
  buffer_trim_right(self);
}

/*
 * Fill the buffer with `c`.
 */

void
buffer_fill(buffer_t *self, int c) {
  memset(self->data, c, self->size);
}

/*
 * Fill the buffer with 0.
 */

void
buffer_clear(buffer_t *self) {
  buffer_fill(self, 0);
  self->len = 0;
}

/*
 * Print a hex dump of the buffer.
 */

void
buffer_print(buffer_t *self) {
  dlog("size=%d, len=%d, content=[%s]", self->size, self->len, self->data);
}

int IsPowerOf2(size_t size)
{
  return (size & (size - 1)) == 0;
}
size_t RoundupPowOf2(size_t size)
{
  if(IsPowerOf2(size))
    return size;
  if(size >= 0x80000000)
    return 0x80000000;
  size_t i = 0;
  for(i = 0; size > 0; ++i)
    size >>= 1;
  size_t ret = 1 << (i);
  return ret;
}
