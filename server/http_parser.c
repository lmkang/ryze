#include <string.h>
#include "http_parser.h"

static const char *token_char_map = 
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
    "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static int str_parse_hex(const char *data, int data_len, size_t *result) {
    *result = 0;
    char *p = (char *) data, *end = (char *) data + data_len;
    size_t base = 0;
    int n = 0;
    char c;
    for(; p != end; p++) {
        c = *p;
        if(c >= '0' && c <= '9') {
            n = c - '0';
        } else if(c >= 'a' && c <= 'f') {
            n = c - 'a' + 0xa;
        } else if(c >= 'A' && c <= 'F') {
            n = c - 'A' + 0xa;
        } else {
            return -1;
        }
        *result += n << base;
        base += 4;
    }
    return 0;
}

int http_parse_request_line(char *data, size_t data_len, 
        char **method, size_t *method_len, char **path, size_t *path_len, 
        int *minor_version, size_t *read_len) {
    *read_len = 0;
    char *pos[3];
    char *p = data, *end = data + data_len;
    int i = 0;
    for(; p != end; p++) {
        if(*p == ' ') {
            pos[i++] = p;
            if(i == 2) {
                break;
            }
        }
    }
    for(; p != end; p++) {
        if(*p == '\r' && ++p != end && *p == '\n') {
            pos[i++] = p;
            break;
        }
    }
    if(i == 3) {
        // " HTTP/1.1\r\n"
        if(pos[2] - pos[1] != 10) {
            return -1;
        }
        char *p = pos[1];
        if(*++p != 'H' || *++p != 'T' || *++p != 'T' || *++p != 'P' 
                || *++p != '/' || *++p != '1' || *++p != '.') {
            return -1;
        }
        *method = data;
        *method_len = pos[0] - *method;
        *path = pos[0] + 1;
        *path_len = pos[1] - *path;
        *minor_version = *++p - '0';
        *read_len = pos[2] - data + 1;
        return 0;
    }
    return 1;
}

int http_parse_header(char *data, size_t data_len, 
        struct http_header *headers, int max_header, 
        int *nheader, size_t *read_len) {
    *nheader = 0;
    *read_len = 0;
    char *p = data, *end = data + data_len, 
        *p1 = data, *p2 = NULL, *p3 = NULL, *t;
    struct http_header *h;
    int i = 0, is_token;
    for(; p != end;) {
        t = p1;
        is_token = 1;
        for(;; t++) {
            // token = 1*<any CHAR except CTLs or separators>
            if(!token_char_map[(unsigned char) *t]) {
                if(*t == ':') {
                    p2 = t;
                    break;
                } else {
                    is_token = 0;
                }
            }
        }
        for(;; t++) {
            if(*t == '\r' && ++t != end && *t == '\n') {
                p3 = t;
                break;
            }
        }
        if(p3) {
            p = t;
            if(*p1 != ' ' && *p1 != '\t') {
                if(is_token && p2 && i < max_header) {
                    h = &headers[i];
                    h->name = p1;
                    h->name_len = p2 - p1;
                    t = p2 + 1;
                    while(*t == ' ' || *t == '\t') {
                        t++;
                    }
                    h->value = t;
                    t = p3 - 2;
                    while(*t == ' ' || *t == '\t') {
                        t--;
                    }
                    h->value_len = t + 1 - h->value;
                    *nheader = ++i;
                }
            } else if(i > 0) {
                // line folding
                t = p1 + 1;
                int len = p - t - 1;
                if(len > 0) {
                    h = &headers[i - 1];
                    memmove(h->value + h->value_len, t, len);
                    h->value_len += len;
                }
            }
            t = ++p;
            *read_len = t - data;
            if(t != end) {
                if(*t == '\r' && ++p != end && *p == '\n') {
                    *read_len = p - data + 1;
                    return 0;
                }
                p1 = t;
                p2 = NULL;
                p3 = NULL;
            }
        } else {
            return -1;
        }
    }
    return 1;
}

int http_parse_chunked(char *data, size_t data_len, 
        char **body, size_t *body_len, size_t *read_len) {
    *body = data;
    *body_len = 0;
    *read_len = 0;
    char *p1 = data, *p2 = NULL;
    size_t n, s;
    int d;
    for(size_t i = 0; i < data_len; i++) {
        // chunk-extension
        if(data[i] == ';') {
            p2 = data + i;
        } else if(data[i] == '\r') {
            d = (p2 ? p2 : data + i) - p1;
            if(d > 0 && str_parse_hex(p1, d, &n) == 0) {
                s = i + 2 + n + 2;
                if(s <= data_len) {
                    if(n > 0) {
                        memmove(*body + *body_len, data + i + 2, n);
                        *body_len += n;
                        *read_len = s;
                    } else {
                        *read_len = i + 2;
                        break;
                    }
                    p1 = data + s;
                    p2 = NULL;
                    i = s;
                } else {
                    return 1;
                }
            } else {
                return -1;
            }
        }
    }
    return 0;
}
