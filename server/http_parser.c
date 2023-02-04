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
    size_t base = 0;
    int n = 0;
    for(int i = data_len - 1; i >= 0; i--) {
        if(data[i] >= '0' && data[i] <= '9') {
            n = data[i] - '0';
        } else if(data[i] >= 'a' && data[i] <= 'f') {
            n = data[i] - 'a' + 0xa;
        } else if(data[i] >= 'A' && data[i] <= 'F') {
            n = data[i] - 'A' + 0xa;
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
    int j = 0;
    for(size_t i = 0; i < data_len; i++) {
        if(data[i] == ' ') {
            pos[j++] = data + i;
        } else if(data[i] == '\r') {
            if(j == 2 && i + 1 < data_len && data[i + 1] == '\n') {
                pos[j++] = data + i;
                break;
            } else {
                return -1;
            }
        }
    }
    if(j == 3) {
        if(pos[2] - pos[1] != 9) {
            return -1;
        }
        char *p = pos[1];
        if(p[1] != 'H' || p[2] != 'T' || p[3] != 'T' || p[4] != 'P' 
                || p[5] != '/' || p[6] != '1' || p[7] != '.') {
            return -1;
        }
        *method = data;
        *method_len = pos[0] - *method;
        *path = pos[0] + 1;
        *path_len = pos[1] - *path;
        *minor_version = *(pos[1] + 8) - '0';
        *read_len = pos[2] + 1 - data + 1;
        return 0;
    }
    return 1;
}

int http_parse_header(char *data, size_t data_len, 
        struct http_header *headers, int max_header, 
        int *nheader, size_t *read_len) {
    *nheader = 0;
    *read_len = 0;
    struct http_header *h;
    char *p, *p1 = data, *p2 = NULL;
    int n, is_token;
    for(size_t i = 0, j = 0; i < data_len; i++) {
        if(data[i] == ':') {
            p2 = data + i;
        } else if(data[i] == '\r' && i + 1 < data_len && data[i + 1] == '\n') {
            if(p1[0] != ' ' && p1[0] != '\t') {
                if(p2 != NULL) {
                    if(j == max_header) {
                        return 2;
                    }
                    // token = 1*<any CHAR except CTLs or separators>
                    is_token = 1;
                    for(n = 0; n < p2 - p1; n++) {
                        if(!token_char_map[(unsigned char) p1[n]]) {
                            is_token = 0;
                            break;
                        }
                    }
                    if(is_token) {
                        h = &headers[j];
                        h->name = p1;
                        h->name_len = p2 - p1;
                        p = p2 + 1;
                        while(*p == ' ' || *p == '\t') {
                            p++;
                        }
                        h->value = p;
                        p = data + i - 1;
                        while(*p == ' ' || *p == '\t') {
                            p--;
                        }
                        h->value_len = p + 1 - h->value;
                        *nheader = ++j;
                    }
                }
            } else if(j > 0) {
                // line folding
                p = p1 + 1;
                int len = data + i - p;
                if(len > 0) {
                    h = &headers[j - 1];
                    memmove(h->value + h->value_len, p, len);
                    h->value_len += len;
                }
            }
            if(i + 3 < data_len && data[i + 2] == '\r' && data[i + 3] == '\n') {
                *read_len = i + 2 + 2;
                return 0;
            }
            *read_len = i + 2;
            p1 = data + i + 2;
            p2 = NULL;
            i++;
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
