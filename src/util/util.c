#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static char *str_split(char *str, int delim, int *count) {
    char *last = str;
    size_t len = strlen(str);
    if(count) {
        *count = 1;
    }
    for(size_t i = 0; i < len; i++) {
        if(str[i] == delim) {
            str[i] = '\0';
            if(i + 1 < len) {
                last = str + i + 1;
                if(count) {
                    *count += 1;
                }
            }
            continue;
        }
    }
    return last;
}

int is_http_path(const char *path) {
    return strstr(path, "https://") == path
        || strstr(path, "http://") == path;
}

int is_absolute_path(const char *path) {
    return path[0] == '/' || is_http_path(path);
}

char *dirname(const char *path) {
    size_t len = strlen(path);
    int is_http = 1;
    size_t start = 0;
    if(strstr(path, "https://") == path) {
        start = 8;
    } else if(strstr(path, "http://") == path) {
        start = 7;
    } else {
        is_http = 0;
    }
    size_t i;
    for(i = len - 1; i >= start; i--) {
        if(path[i] == '/') {
            if(i == len - 1) {
                continue;
            } else {
                break;
            }
        }
    }
    if(i == start) {
        if(is_http) {
            size_t end = len;
            for(size_t i = start; i < len; i++) {
                if(path[i] == '/' || path[i] == '?') {
                    end = i;
                    break;
                }
            }
            char *str = malloc(end + 1);
            memcpy(str, path, end);
            str[end] = '\0';
            return str;
        } else {
            char *str = malloc(2);
            str[0] = path[i] == '/' ? '/' : '.';
            str[1] = '\0';
            return str;
        }
    } else {
        char *str = malloc(i + 1);
        memcpy(str, path, i);
        str[i] = '\0';
        return str;
    }
}

char *read_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        printf("Error: can not open file: %s\n", path);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    char *str = malloc(size + 1);
    str[size] = '\0';
    for(size_t i = 0; i < size; ) {
        i += fread(str + i, 1, size - i, fp);
        if(ferror(fp)) {
            fclose(fp);
            free(str);
            return NULL;
        }
    }
    fclose(fp);
    return str;
}

char *normalize_path(const char *path, const char *basedir) {
    if(is_absolute_path(path)) {
        size_t len = strlen(path);
        char *str = malloc(len + 1);
        str[len] = '\0';
        memcpy(str, path, len);
        return str;
    }
    
    size_t len = strlen(basedir);
    size_t start = 0;
    if(strstr(basedir, "https://") == basedir) {
        start = 8;
    } else if(strstr(basedir, "http://") == basedir) {
        start = 7;
    }
    for(size_t i = start; i < len; i++) {
        if(basedir[i] == '/') {
            start = i;
            break;
        }
    }
    size_t len2 = strlen(path);
    char *str = malloc(len2 + len - start + 1);
    memcpy(str, basedir + start + 1, len - start - 1);
    str[len - start - 1] = '/';
    memcpy(str + len - start, path, len2);
    str[len2 + len - start] = '\0';
    
    int count;
    str_split(str, '/', &count);
    char **stack = malloc(sizeof(char *) * count);
    int index = 0;
    size_t total = 0;
    char *prev = str;
    char *p = str;
    for(int i = 0; i < count; i++) {
        len = strlen(p);
        if(strcmp(p, "..")) {
            if(strcmp(p, ".")) {
                stack[index++] = p;
                total += len + 1;
            }
        } else {
            index -= 1;
            total -= strlen(prev) + 1;
        }
        prev = p;
        p += len + 1;
    }
    total += start;
    char *result = malloc(total + 1);
    memcpy(result, basedir, start);
    for(int i = 0, j = start; i < index; i++) {
        len = strlen(stack[i]);
        result[j++] = '/';
        memcpy(result + j, stack[i], len);
        j += len;
    }
    result[total] = '\0';
    free(stack);
    free(str);
    return result;
}

char *http_get(const char *url) {
    int port;
    size_t start = 0;
    if(strstr(url, "https://") == url) {
        start = 8;
        port = 443;
    } else if(strstr(url, "http://") == url) {
        start = 7;
        port = 80;
    } else {
        printf("Error: unknown url: %s\n", url);
        return NULL;
    }
    size_t len = strlen(url);
    size_t end = len;
    for(size_t i = start; i < len; i++) {
        if(url[i] == '/') {
            end = i;
            break;
        }
    }
    
    len = len - end;
    char *suffix = malloc(len + 1);
    memcpy(suffix, url + end, len);
    suffix[len] = '\0';
    len = end - start;
    char *host = malloc(len + 1);
    memcpy(host, url + start, len);
    host[len] = '\0';
    const char *str = "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Accept: */*\r\n"
        "Cache-Control: no-cache\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/99.0.4844.82 Safari/537.36\r\n"
        "\r\n";
    len = strlen(str) + strlen(host) + strlen(suffix);
    char *header = malloc(len + 1);
    sprintf(header, str, suffix, host);
    free(suffix);
    len = strlen(host);
    for(size_t i = 0; i < len; i++) {
        if(host[i] == ':') {
            port = atoi(host + i + 1);
            break;
        }
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    struct hostent *h = gethostbyname(host);
    if(h != NULL) {
        addr.sin_addr.s_addr = inet_addr(h->h_addr);
        free(h);
    } else {
        str_split(host, ':', NULL);
        addr.sin_addr.s_addr = inet_addr(host);
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    send(sockfd, header, strlen(header), 0);
    free(host);
    free(header);
    
    size_t chunk_size = 1024;
    size_t buf_size = chunk_size;
    char *buf = malloc(buf_size + 1);
    char *result = NULL;
    char *p;
    len = 0;
    while(1) {
        ret = recv(sockfd, buf + len, chunk_size, 0);
        if(ret > 0) {
            len += ret;
        }
        buf[buf_size] = '\0';
        if(!(p = strstr(buf, "\r\n\r\n"))) {
            if(ret <= 0) {
                break;
            }
            buf_size += chunk_size;
            buf = realloc(buf, buf_size);
            continue;
        }
        end = p - buf + 4;
        if((p = strstr(buf, "Content-Length: "))) {
            start = p - buf + 16;
            for(size_t i = start; i < buf_size - 1; i++) {
                if(buf[i] == '\r' && buf[i + 1] == '\n') {
                    buf[i] = '\0';
                    break;
                }
            }
            long int content_len = atol(buf + start);
            result = malloc(content_len + 1);
            size_t index = 0;
            if(len > end) {
                memcpy(result + index, buf + end, len - end);
                index += len - end;
            }
            while(1) {
                ret = recv(sockfd, result + index, content_len - index, 0);
                if(ret <= 0) {
                    break;
                }
                index += ret;
            }
            result[content_len] = '\0';
            break;
        } else {
            // TODO: Transfer-Encoding: chunked
            printf("TODO: Transfer-Encoding: chunked\n");
            break;
        }
    }
    close(sockfd);
    free(buf);
    return result;
}
