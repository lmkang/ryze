#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

struct http_header {
    char *name;
    size_t name_len;
    char *value;
    size_t value_len;
};

static int str_parse_int(const char *data, size_t data_len, int radix, size_t *result) {
    *result = 0;
    int ret = 0;
    size_t base = 1;
    size_t i = data_len - 1;
    int n = 0;
    while(i >= 0) {
        if(data[i] >= '0' && data[i] <= '9') {
            n = data[i] - '0';
        } else if(data[i] >= 'a' && data[i] <= 'z') {
            n = data[i] - 'a' + 10;
        } else if(data[i] >= 'A' && data[i] <= 'Z') {
            n = data[i] - 'A' + 10;
        } else {
            ret = -1;
            break;
        }
        *result += n * base;
        if(i == 0) {
            break;
        }
        i--;
        base *= radix;
    }
    return ret;
}

int http_parse_request_line(char *data, size_t data_len, 
        char **method, size_t *method_len, char **path, size_t *path_len, 
        int *minor_version, size_t *read_len) {
    *read_len = 0;
    char *pos[3];
    int j = 0;
    for(size_t i = 0; i < data_len && j < 3; i++) {
        if(data[i] == ' ' || (data[i] == '\r' && i + 1 < data_len && data[i + 1] == '\n')) {
            pos[j++] = data + i;
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
    for(size_t i = 0, j = 0; i < data_len; i++) {
        if(data[i] == ':') {
            p2 = data + i;
        } else if(data[i] == '\r' && i + 1 < data_len && data[i + 1] == '\n') {
            if(p1[0] != ' ' && p1[0] != '\t') {
                if(p2 != NULL) {
                    if(j == max_header) {
                        return 2;
                    }
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
            } else if(j > 0) {
                // line folding
                p = p1 + 1;
                while(*p == ' ' || *p == '\t') {
                    p++;
                }
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
    char *p1 = data, *p2 = NULL, *p3;
    size_t n;
    for(size_t i = 0; i < data_len; i++) {
        // chunk-extension
        if(data[i] == ';') {
            p2 = data + i;
        } else if(data[i] == '\r') {
            p3 = p2 == NULL ? data + i : p2;
            if(str_parse_int(p1, p3 - p1, 16, &n) == 0) {
                if(i + 1 + n + 2 < data_len) {
                    if(n > 0) {
                        memmove(*body + *body_len, data + i + 2, n);
                        *body_len += n;
                        *read_len = i + 2 + n + 2;
                    } else {
                        break;
                    }
                    p1 = data + i + 2 + n + 2;
                    p2 = NULL;
                    i += 1 + n + 2;
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

int http_parse_request(char *data, size_t data_len, 
        char **method, size_t *method_len, char **path, size_t *path_len, 
        int *minor_version, struct http_header *headers, int max_header, 
        int *nheader, char **body, size_t *body_len, size_t *read_len) {
    *read_len = 0;
    size_t len;
    int ret = http_parse_request_line(data, data_len, method, method_len, 
        path, path_len, minor_version, &len);
    if(ret != 0) {
        return ret;
    }
    *read_len += len;
    ret = http_parse_header(data + *read_len, data_len - *read_len, 
        headers, max_header, nheader, &len);
    if(ret != 0) {
        return ret;
    }
    *read_len += len;
    *body_len = 0;
    if(*read_len < data_len) {
        *body = data + *read_len;
        *body_len = data_len - *read_len;
    }
    return 0;
}

int main(int argc, char **argv) {
    char *data = "GET /cookies HTTP/1.1\r\nHost: 127.0.0.1:8090\r\nConnection: keep-alive\r\nCache-Control: max-age=0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17\r\nAccept-Encoding: gzip,deflate,sdch\r\nAccept-Language: en-US,en;q=0.8\r\nAccept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\nCookie: name=wookie\r\n\r\n";
    size_t data_len = strlen(data);
    char *method, *path, *body;
    size_t method_len, path_len, body_len, read_len;
    int minor_version, ret;
    int max_header = 64, nheader;
    struct http_header *headers = malloc(sizeof(struct http_header) * max_header);
    float start, end;
    start = (float) clock() / CLOCKS_PER_SEC;
    for(int i = 0; i < 1000000; i++) {
        ret = http_parse_request(data, data_len, &method, &method_len, &path, &path_len, 
            &minor_version, headers, max_header, &nheader, &body, &body_len, &read_len);
        assert(ret == 0);
    }
    end = (float) clock() / CLOCKS_PER_SEC;
    printf("Elapsed %f seconds.\n", (end - start));
    free(headers);
    return 0;
}
