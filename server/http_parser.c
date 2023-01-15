#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct http_request_line {
    char *method;
    size_t method_len;
    char *path;
    size_t path_len;
    char *version;
    size_t version_len;
};

struct http_header {
    char *name;
    size_t name_len;
    char *value;
    size_t value_len;
};

struct http_resolved_header {
    size_t content_length;
    int transfer_encoding;
    int connection;
};

static int http_strcmp_ignore(const char *data, size_t data_len, 
        const char *value, size_t value_len) {
    if(data_len != value_len) {
        return 0;
    }
    int ret = 1;
    int d1, d2, d3;
    for(size_t i = 0; i < data_len; i++) {
        d1 = data[i] - value[i];
        if(diff == 0) {
            continue;
        }
        d2 = data[i] - 'a';
        d3 = data[i] - 'A';
        if(d2 >= 0 && d2 <= 25 || d3 >= 0 && d3 <= 25) {
            continue;
        }
        ret = 0;
        break;
    }
    return ret;
}

static size_t http_parse_int(const char *data, size_t data_len, int radix) {
    size_t result = 0;
    size_t base = 1;
    int n = 0;
    size_t i = data_len - 1;
    while(i >= 0) {
        if(data[i] >= '0' && data[i] <= '9') {
            n = data[i] - '0';
        } else if(data[i] >= 'a' && data[i] <= 'z') {
            n = data[i] - 'a' + 10;
        } else if(data[i] >= 'A' && data[i] <= 'Z') {
            n = data[i] - 'A' + 10;
        }
        result += n * base;
        if(i == 0) {
            break;
        }
        i--;
        base *= radix;
    }
    return result;
}

int http_parse_request_line(const char *data, size_t data_len, 
        struct http_request_line *request_line, size_t *read_len) {
    char *pos[3];
    int j = 0;
    for(size_t i = 0; i < data_len && j < 3; i++) {
        if(data[i] == ' ' || data[i] == '\r') {
            pos[j++] = (char *) data + i;
        }
    }
    if(j == 3) {
        request_line->method = (char *) data;
        request_line->method_len = pos[0] - request_line->method;
        request_line->path = pos[0] + 1;
        request_line->path_len = pos[1] - request_line->path;
        request_line->version = pos[1] + 1 + 5;
        request_line->version_len = pos[2] - request_line->version;
        *read_len = pos[2] + 1 - data + 1;
        return 0;
    }
    return 1;
}

int http_parse_header(const char *data, size_t data_len, 
        struct http_header *headers, size_t max_header, size_t *nheader, 
        struct http_header_info *header_info, size_t *read_len) {
    header_info->content_length = 0;
    header_info->connection = kKeepAlive;
    header_info->transfer_encoding = kNone;
    for(size_t i = 0, j = 0, index = 0; i < data_len - 4; i++) {
        if(data[i] == '\r' && data[i + 1] == '\n') {
            *nheader += 1;
            *read_len = i + 2;
            if(index == max_header) {
                max_header = (max_header * 3) >> 2;
                headers = realloc(headers, max_header);
            }
            for(size_t k = j; k < i; k++) {
                if(data[k] == ':') {
                    headers[index].name = (char *) data + j;
                    headers[index].name_len = k - j;
                    while(data[++k] == ' ' || data[k] == '\t') {
                        // skip spaces or tabs
                    }
                    headers[index].value = (char *) data + k;
                    headers[index].value_len = i - k;
                    if(http_strcmp_ignore(headers[index].name, 
                        headers[index].name_len, "Content-Length", 14)) {
                        header_info->content_length = http_parse_int(
                            headers[index].value, headers[index].value_len, 10);
                    }
                    if(http_strcmp_ignore(headers[index].name, 
                        headers[index].name_len, "Transfer-Encoding", 17)) {
                        if(http_strcmp_ignore(headers[index].value, 
                            headers[index].value_len, "chunked", 7)) {
                            header_info->transfer_encoding = kChunked;
                        }
                    }
                    if(http_strcmp_ignore(headers[index].name, 
                        headers[index].name_len, "Connection", 10)) {
                        if(http_strcmp_ignore(headers[index].value, 
                            headers[index].value_len, "close", 7)) {
                            header_info->connection = kClose;
                        }
                    }
                    break;
                }
            }
            j = i + 2;
            index++;
            if(data[i + 2] == '\r' && data[i + 3] == '\n') {
                *read_len += 2;
                return 0;
            }
            i++;
        }
    }
    return 1;
}

int http_parse_body(const char *data, size_t data_len, 
        size_t content_len, int is_chunked, size_t *read_len) {
    return 0;
}

int main(int argc, char **argv) {
    const char *data = "POST /aa/bb/cc HTTP/1.1\r\nContent-Length: 11\r\n\r\nhello world";
    size_t data_len = strlen(data);
    struct http_request_line *request_line = malloc(sizeof(struct http_request_line));
    size_t read_len = 0;
    int ret = http_parse_request_line(data, data_len, request_line, &read_len);
    if(ret == 0) {
        printf("method: %.*s\n", request_line->method_len, request_line->method);
        printf("path: %.*s\n", request_line->path_len, request_line->path);
        printf("version: %.*s\n", request_line->version_len, request_line->version);
        
        size_t max_header = 16;
        struct http_header *headers = malloc(sizeof(struct http_header) * max_header);
        struct http_header_info *header_info = malloc(sizeof(struct http_header_info));
        size_t nheader = 0;
        size_t read_len2 = 0;
        ret = http_parse_header(data + read_len, data_len - read_len, 
            headers, max_header, &nheader, header_info, &read_len2);
        if(ret == 0) {
            printf("num of header: %d\n", nheader);
            printf("headers : \n");
            for(int i = 0; i < nheader; i++) {
                printf("name: %.*s, value: %.*s\n", 
                    headers[i].name_len, headers[i].name, 
                    headers[i].value_len, headers[i].value);
            }
            printf("content length: %d\n", header_info->content_length);
            printf("chunked: %d\n", http_is_chunked(header_info));
            printf("keep alive: %d\n", http_is_keep_alive(request_line, header_info));
        }
        free(headers);
        free(header_info);
    }
    free(request_line);
    return 0;
}
