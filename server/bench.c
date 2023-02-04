#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "http_parser.h"

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
    body_len = 0;
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
    int nheader;
    struct http_header headers[64];
    float start, end;
    start = (float) clock() / CLOCKS_PER_SEC;
    for(int i = 0; i < 100000; i++) {
        ret = http_parse_request(data, data_len, &method, &method_len, &path, &path_len, 
            &minor_version, headers, 64, &nheader, &body, &body_len, &read_len);
        assert(ret == 0);
    }
    end = (float) clock() / CLOCKS_PER_SEC;
    printf("Elapsed %f seconds.\n", (end - start));
    return 0;
}
