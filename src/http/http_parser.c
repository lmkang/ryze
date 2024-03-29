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

static int str_parse_hex(const char *data, int data_len) {
    int result = 0;
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
        result += n << base;
        base += 4;
    }
    return result;
}

int http_parse_request_line(char *data, size_t data_len, 
        char **method, size_t *method_len, char **path, 
        size_t *path_len, int *minor_version) {
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
        return pos[2] - data + 1;
    }
    return -2;
}

int http_parse_header(char *data, size_t data_len, 
        struct http_header *headers, int max_header, int *header_len) {
    *header_len = 0;
    int read_len = -2;
    char *p = data, *end = data + data_len, 
        *p1 = data, *p2 = NULL, *p3 = NULL, *t;
    struct http_header *h;
    int i = 0, is_token;
    for(; p != end;) {
        is_token = 1;
        for(t = p1; ; t++) {
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
                    *header_len = ++i;
                }
            } else if(i > 0) {
                // line folding
                int len = p - p1 - 1;
                if(len > 0) {
                    headers[i - 1].value_len += len;
                }
            }
            t = ++p;
            read_len = t - data;
            if(t != end) {
                if(*t == '\r' && ++p != end && *p == '\n') {
                    break;
                }
                p1 = t;
                p2 = NULL;
                p3 = NULL;
            }
        } else {
            return -1;
        }
    }
    return read_len;
}

int http_decode_chunked(char *data, size_t data_len, int *offset) {
    char *p1 = data, *p2 = NULL, *p = data, *end = data + data_len;
    for(; p != end; p++) {
        if(*p == ';') {
            p2 = p;
        } else if(*p == '\r') {
            int n = p - p1;
            if(n > 0 && (n = str_parse_hex(p1, n)) > 0) {
                *offset = p - data + 2;
                return n;
            } else {
                return -1;
            }
        }
    }
    return -1;
}
