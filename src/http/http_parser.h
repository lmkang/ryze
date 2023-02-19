#ifndef RYZE_HTTP_PARSER_H
#define RYZE_HTTP_PARSER_H

struct http_header {
    char *name;
    size_t name_len;
    char *value;
    size_t value_len;
};

int http_parse_request_line(char *data, size_t data_len, 
    char **method, size_t *method_len, char **path, 
    size_t *path_len, int *minor_version);

int http_parse_header(char *data, size_t data_len, 
    struct http_header *headers, int max_header, int *header_len);

int http_decode_chunked(char *data, size_t data_len, int *offset);

#endif
