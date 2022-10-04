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

struct url_struct {
    char *protocol;
    char *host;
    char *hostname;
    int port;
    char *path;
    char *pathname;
    char *search;
    char *hash;
};

struct url_struct *url_parse(const char *url) {
    struct url_struct *url_s = malloc(sizeof(struct url_struct));
    url_s->search = NULL;
    url_s->hash = NULL;
    
    #define RYZE_URL_COPY_STR(dst, index, src, len) \
        dst = malloc((len) + 1); \
        memcpy(dst + (index), src, len); \
        dst[len] = '\0'
    
    // protocol
    char *p = strstr(url, "//");
    RYZE_URL_COPY_STR(url_s->protocol, 0, url, p - url);
    
    // host
    int len = strlen(url);
    int start = p - url + 2;
    int end = -1;
    if(!(p = strstr(url + start, "/"))) {
        if(!(p = strstr(url, "?"))) {
            p = strstr(url, "#");
        }
    }
    end = p ? p - url : len;
    RYZE_URL_COPY_STR(url_s->host, 0, url + start, end - start);
    
    // hostname
    p = strstr(url_s->host, ":");
    RYZE_URL_COPY_STR(url_s->hostname, 0, url_s->host, 
        p ? p - url_s->host : strlen(url_s->host));
    
    // port
    if(p) {
        url_s->port = atoi(p + 1);
    } else {
        url_s->port = strcmp(url_s->protocol, "http:") ? 443 : 80;
    }
    
    // path
    if(end < len) {
        p = strstr(url, "#");
        if(url[end] == '/') {
            RYZE_URL_COPY_STR(url_s->path, 0, url + end, 
                p ? p - url - end : len - end);
        } else {
            RYZE_URL_COPY_STR(url_s->path, 1, url + end, 
                1 + (p ? p - url - end : len - end));
            url_s->path[0] = '/';
        }
    } else {
        RYZE_URL_COPY_STR(url_s->path, 0, "/", 1);
    }
    
    // pathname
    p = strstr(url_s->path, "?");
    RYZE_URL_COPY_STR(url_s->pathname, 0, url_s->path, 
        p ? p - url_s->path : strlen(url_s->path));
    
    // search
    if((p = strstr(url, "?"))) {
        start = p - url;
        p = strstr(url, "#");
        RYZE_URL_COPY_STR(url_s->search, 0, url + start, 
            p ? p - url - start : len - start);
    }
    
    // hash
    if((p = strstr(url, "#"))) {
       RYZE_URL_COPY_STR(url_s->hash, 0, p, len - (p - url)); 
    }
    
    #undef RYZE_URL_COPY_STR
    return url_s;
}

void url_free(struct url_struct *ptr) {
    free(ptr->protocol);
    free(ptr->host);
    free(ptr->hostname);
    free(ptr->path);
    free(ptr->pathname);
    if(ptr->search) {
        free(ptr->search);
    }
    if(ptr->hash) {
        free(ptr->hash);
    }
    free(ptr);
}

char *str_split(char *str, const char *delim) {
    char *last = str;
    size_t len = strlen(delim);
    size_t index = 0;
    char *p = NULL;
    while(1) {
        if((p = strstr(str + index, delim))) {
            last = p + len;
            p[0] = '\0';
            index = p - str + len;
        } else {
            break;
        }
    }
    return last;
}

char *dirname(const char *str) {
    size_t len = strlen(str);
    long int index = -1;
    for(long int i = len - 1; i >= 0; i--) {
        if(str[i] == '/') {
            if(i == len - 1 || str[i + 1] == '/') {
                continue;
            }
            index = i;
            break;
        }
    }
    if(index <= 0) {
        char *r = malloc(2);
        strcpy(r, index == -1 ? "." : "/");
        return r;
    } else {
        char *r = malloc(index + 1);
        memcpy(r, str, index);
        r[index] = '\0';
        return r;
    }
}

char *normalize_path(const char *basedir, const char *path) {
    size_t len1 = strlen(basedir);
    size_t len2 = strlen(path);
    char *full_path = malloc(len1 + len2 + 2);
    strcpy(full_path, basedir);
    full_path[len1] = '/';
    strcpy(full_path + len1 + 1, path);
    char *last = str_split(full_path, "/");
    char *p = full_path;
    int len = 1024;
    char **stack = malloc(sizeof(char *) * len);
    int index = 0;
    while(1) {
        if(strcmp(p, "..")) {
            if(strcmp(p, "") && strcmp(p, ".")) {
                if(index >= len) {
                    len += 1024;
                    stack = realloc(stack, sizeof(char *) * len);
                }
                stack[index++] = p;
            }
        } else {
            index--;
        }
        if(p == last) {
            break;
        }
        p += strlen(p) + 1;
    }
    char *result = malloc(len1 + len2 + 2);
    for(int i = 0, j = 0, k; i < index; i++) {
        result[j++] = '/';
        strcpy(result + j, stack[i]);
        k = strlen(stack[i]);
        j += k;
    }
    free(stack);
    free(full_path);
    return result;
}

char *http_get(const char *url, const char *header) {
    struct url_struct *url_s = url_parse(url);
    const char *fmt = "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "%s\r\n";
    char *headers = malloc(strlen(fmt) + strlen(url_s->path) + 
        strlen(url_s->host) + (header ? strlen(header) : 0) + 1);
    sprintf(headers, fmt, url_s->path, url_s->host, header ? header : "");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(url_s->port);
    int sockfd = -1;
    char *buf = NULL;
    do {
        struct hostent *he = gethostbyname(url_s->hostname);
        if(!he) {
            printf("fail to gethostbyname(), %d: %s\n", errno, strerror(errno));
            break;
        }
        addr.sin_addr.s_addr = 
            inet_addr(inet_ntoa(*(struct in_addr *) he->h_addr_list[0]));
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            printf("fail to socket(), %d: %s\n", errno, strerror(errno));
            break;
        }
        int ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
        if(ret != 0) {
            printf("fail to connect(), %d: %s\n", errno, strerror(errno));
            break;
        }
        send(sockfd, headers, strlen(headers), 0);
        size_t len = 1024;
        char *body = malloc(len);
        size_t nread = 0;
        while(1) {
            ret = recv(sockfd, body + nread, 1024, 0);
            if(ret <= 0) {
                break;
            }
            nread += ret;
            len += 1024;
            body = realloc(body, len);
        }
        size_t body_start = 0;
        for(size_t i = 0; i < nread - 4; i++) {
            if(body[i] == '\r' && body[i + 1] == '\n'
                    && body[i + 2] == '\r' && body[i + 3] == '\n') {
                body_start = i + 4;
                break;
            }
        }
        buf = malloc(nread - body_start + 1);
        memcpy(buf, body + body_start, nread - body_start);
        buf[nread - body_start] = '\0';
        free(body);
    } while(0);
    if(sockfd >= 0) {
        close(sockfd);
    }
    free(headers);
    free(url_s);
    return buf;
}

int main(int argc, char **argv) {
    char *str = malloc(32);
    strcpy(str, "./a");
    /*char *last = str_split(str, "$$");
    char *p = str;
    while(1) {
        if(p == last) {
            printf("str: %s\n", p);
            break;
        }
        printf("str: %s\n", p);
        p += strlen(p) + 2;
    }*/
    
    /*char *dir = dirname(str);
    printf("dir: %s\n", dir);
    free(dir);
    free(str);*/
    
    /*char *path = normalize_path("/a/b/c", "../../e/f/11.js");
    printf("path: %s\n", path);
    free(path);*/
    
    /*struct url_struct *url = url_parse("http://www.baidu.com/a/b/?aa=aa&bb=bb#cc");
    printf("protocol: %s, host: %s, hostname: %s, port: %d, path: %s, pathname: %s, search: %s, hash: %s\n", url->protocol, url->host, url->hostname, url->port, url->path, url->pathname, url->search, url->hash);
    url_free(url);*/
    
    char *buf = http_get("https://www.unpkg.com/dplayer@1.27.0/dist/DPlayer.min.js", NULL);
    if(buf) {
        printf("%s\n", buf);
        free(buf);
    }
    return 0;
}
