#ifndef RYZE_HTTP_PARSER_H
#define RYZE_HTTP_PARSER_H

int http_parse_request_line(char *data, size_t data_len, 
    char **method, size_t *method_len, char **path, size_t *path_len, 
    int *minor_version, size_t *read_len);

int http_parse_header(char *data, size_t data_len, 
    struct http_header *headers, int max_header, 
    int *nheader, size_t *read_len);

int http_parse_chunked(char *data, size_t data_len, 
    char **body, size_t *body_len, size_t *read_len);

#endif
