#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int http_parse_request_line(const char *data, size_t data_len, 
        char **method, size_t *method_len, char **path, size_t *path_len, 
        int *minor_version, size_t *read_len) {
    *read_len = 0;
    char *pos[3];
    int j = 0;
    for(size_t i = 0; i < data_len && j < 3; i++) {
        if(data[i] == ' ' || (data[i] == '\r' && i + 1 < data_len && data[i + 1] == '\n')) {
            pos[j++] = (char *) data + i;
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
        *method = (char *) data;
        *method_len = pos[0] - *method;
        *path = pos[0] + 1;
        *path_len = pos[1] - *path;
        *minor_version = *(pos[1] + 8) - '0';
        *read_len = pos[2] + 1 - data + 1;
        return 0;
    }
    return 1;
}

int http_parse_header(const char *data, size_t data_len, 
        struct http_header *headers, size_t max_header, 
        size_t *nheader, size_t *read_len) {
    *nheader = 0;
    *read_len = 0;
    struct http_header *h;
    char *p, *p1 = (char *) data, *p2 = NULL;
    for(size_t i = 0, j = 0; i < data_len; i++) {
        if(data[i] == ':') {
            p2 = (char *) data + i;
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
                    p = (char *) data + i - 1;
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
            p1 = (char *) data + i + 2;
            p2 = NULL;
            i++;
        }
    }
    return 1;
}

int http_parse_chunked(const char *data, size_t data_len, 
        char **buf, size_t *buf_len, size_t *read_len) {
    *buf = (char *) data;
    *buf_len = 0;
    *read_len = 0;
    char *p1 = (char *) data, *p2 = NULL, *p3;
    size_t n;
    for(size_t i = 0; i < data_len; i++) {
        // chunk-extension
        if(data[i] == ';') {
            p2 = (char *) data + i;
        } else if(data[i] == '\r') {
            p3 = (char *) (p2 == NULL ? data + i : p2);
            if(str_parse_int(p1, p3 - p1, 16, &n) == 0) {
                if(i + 1 + n + 2 < data_len) {
                    if(n > 0) {
                        memmove(*buf + *buf_len, data + i + 2, n);
                        *buf_len += n;
                        *read_len = i + 2 + n + 2;
                    } else {
                        break;
                    }
                    p1 = (char *) data + i + 2 + n + 2;
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

int main(int argc, char **argv) {
    char *data = malloc(4096);
    strcpy(data, "POST /aa/bb/cc HTTP/1.1\r\nContent-Length: 1234\r\nX-Test11: aa\r\n bb\r\nX-Test22: cc, dd, ee \r\n\r\n1\r\na\r\n2;name=value\r\nbb\r\n3\r\nccc\r\nd\r\n, hello world\r\n0\r\n\r\n");
    size_t data_len = strlen(data);
    char *method, *path;
    int minor_version;
    size_t method_len, path_len, read_len = 0;
    int ret = http_parse_request_line(data, data_len, &method, &method_len, 
        &path, &path_len, &minor_version, &read_len);
    if(ret == 0) {
        printf("method: %.*s\n", method_len, method);
        printf("path: %.*s\n", path_len, path);
        printf("minor version: %d\n", minor_version);
        
        size_t max_header = 64;
        struct http_header *headers = malloc(sizeof(struct http_header) * max_header);
        size_t nheader = 0;
        size_t len = 0;
        ret = http_parse_header(data + read_len, data_len - read_len, 
            headers, max_header, &nheader, &len);
        if(ret == 0) {
            read_len += len;
            printf("num of header: %d\n", nheader);
            printf("headers : \n");
            for(int i = 0; i < nheader; i++) {
                printf("name: %.*s, value: %.*s\n", 
                    headers[i].name_len, headers[i].name, 
                    headers[i].value_len, headers[i].value);
            }
            
            char *buf;
            size_t buf_len;
            ret = http_parse_chunked(data + read_len, data_len - read_len, 
                &buf, &buf_len, &len);
            if(ret == 0) {
                printf("body: %.*s\n", buf_len, buf);
            } else {
                printf("fail to parse chunked: %d\n", ret);
            }
        } else {
            printf("fail to parse header: %d\n", ret);
        }
        free(headers);
    } else {
        printf("fail to parse request line: %d\n", ret);
    }
    free(data);
    return 0;
}
