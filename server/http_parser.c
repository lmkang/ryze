#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct http_request_line {
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    int minor_version;
};

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
        struct http_request_line *request_line, size_t *read_len) {
    *read_len = 0;
    char *pos[3];
    int j = 0;
    for(size_t i = 0; i < data_len && j < 3; i++) {
        if(data[i] == ' ' || data[i] == '\r') {
            pos[j++] = (char *) data + i;
        }
    }
    if(j == 3) {
        if(pos[2] - data + 1 == data_len) {
            return 1;
        }
        if(*(pos[2] + 1) != '\n' || pos[2] - pos[1] != 9) {
            return -1;
        }
        char *p = pos[1];
        if(*++p != 'H' || *++p != 'T' || *++p != 'T' || *++p != 'P' 
                || *++p != '/' || *++p != '1' || *++p != '.') {
            return -1;
        }
        request_line->method = (char *) data;
        request_line->method_len = pos[0] - request_line->method;
        request_line->path = pos[0] + 1;
        request_line->path_len = pos[1] - request_line->path;
        request_line->minor_version = *(pos[1] + 8) - '0';
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
    char *p1 = (char *) data, *p2 = NULL, *p3 = NULL, *p4 = NULL;
    struct http_header *h;
    for(size_t i = 0, j = 0; i < data_len; i++) {
        if(j == max_header) {
            return 2;
        }
        if(data[i] == ':') {
            p2 = (char *) data + i;
        } else if(p2 != NULL && p3 == NULL && data[i] != ' ' && data[i] != '\t') {
            p3 = (char *) data + i;
        } else if(data[i] == '\r' && i + 1 < data_len && data[i + 1] == '\n') {
            if(p2 != NULL && p3 != NULL && p4 != NULL) {
                h = &headers[j];
                h->name = p1;
                h->name_len = p2 - p1;
                h->value = p3;
                h->value_len = p4 + 1 - p3;
                *nheader = ++j;
            } else if((*p1 == ' ' || *p1 == '\t') && j > 0) {
                // line folding, skip SP and HT, concat value
                char *p = p1 + 1;
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
            p3 = NULL;
            p4 = NULL;
            i++;
        } else if(p3 != NULL && data[i] != ' ' && data[i] != '\t') {
            p4 = (char *) data + i;
        }
    }
    return 1;
}

int http_parse_chunked(const char *data, size_t data_len, size_t *read_len) {
    return 0;
}

int main(int argc, char **argv) {
    char *data = malloc(4096);
    strcpy(data, "POST /aa/bb/cc HTTP/1.1\r\nContent-Length: 1234\r\nX-Test11: aa\r\n bb\r\nX-Test22: cc, dd, ee \r\n\r\nhello world");
    size_t data_len = strlen(data);
    struct http_request_line *request_line = malloc(sizeof(struct http_request_line));
    size_t read_len = 0;
    int ret = http_parse_request_line(data, data_len, request_line, &read_len);
    if(ret == 0) {
        printf("method: %.*s\n", request_line->method_len, request_line->method);
        printf("path: %.*s\n", request_line->path_len, request_line->path);
        printf("minor version: %d\n", request_line->minor_version);
        
        size_t max_header = 64;
        struct http_header *headers = malloc(sizeof(struct http_header) * max_header);
        size_t nheader = 0;
        size_t read_len2 = 0;
        ret = http_parse_header(data + read_len, data_len - read_len, 
            headers, max_header, &nheader, &read_len2);
        if(ret == 0) {
            printf("num of header: %d\n", nheader);
            printf("headers : \n");
            for(int i = 0; i < nheader; i++) {
                printf("name: %.*s, value: %.*s\n", 
                    headers[i].name_len, headers[i].name, 
                    headers[i].value_len, headers[i].value);
            }
        } else {
            printf("fail to parse header: %d\n", ret);
        }
        free(headers);
    }
    free(request_line);
    free(data);
    return 0;
}
