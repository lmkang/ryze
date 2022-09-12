#ifndef RYZE_UTIL_H
#define RYZE_UTIL_H

int is_http_path(const char *path);
int is_absolute_path(const char *path);
char *dirname(const char *path);
char *read_file(const char *path);
char *normalize_path(const char *path, const char *basedir);
char *http_get(const char *url);

#endif
