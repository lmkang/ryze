#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_absolute_path(const char *path) {
    return path[0] == '/'
        || strstr(path, "https://") == path
        || strstr(path, "http://") == path;
}

char *str_split(char *str, int delim, int *count) {
    char *last = str;
    size_t len = strlen(str);
    *count = 1;
    for(size_t i = 1; i < len; i++) {
        if(str[i] == delim) {
            str[i] = '\0';
            if(i + 1 < len) {
                last = str + i + 1;
                *count += 1;
            }
            continue;
        }
    }
    return last;
}

char *dirname(const char *path) {
    size_t len = strlen(path);
    size_t i;
    for(i = len - 1; i >= 0; i--) {
        if(path[i] == '/') {
            if(i == len - 1) {
                continue;
            } else {
                break;
            }
        }
    }
    if(path[i] == '/') {
        if(i == 0) {
            char *str = (char *) malloc(2);
            str[0] = '/';
            str[1] = '\0';
            return str;
        } else {
            char *str = (char *) malloc(i + 1);
            memcpy(str, path, i);
            str[i] = '\0';
            return str;
        }
    } else {
        char *str = (char *) malloc(2);
        str[0] = '.';
        str[1] = '\0';
        return str;
    }
}

char *normalize_path(const char *path, const char *referer) {
    if(is_absolute_path(path)) {
        size_t len = strlen(path);
        char *str = (char *) malloc(len + 1);
        str[len] = '\0';
        memcpy(str, path, len);
        return str;
    }
    
    int is_http = 1;
    size_t start = 0;
    if(strstr(referer, "https://") == referer) {
        start = 8;
    } else if(strstr(referer, "https://") == referer) {
        start = 7;
    } else {
        is_http = 0;
    }
    int len = strlen(referer);
    for(size_t i = start; i < len; i++) {
        if(referer[i] == '/') {
            start = i;
            break;
        }
    }
    size_t end = len;
    for(size_t i = start; i < len; i++) {
        if(referer[i] == '?') {
            end = i;
            break;
        }
    }
    len = end - start;
    char *tmp = (char *) malloc(len + 1);
    memcpy(tmp, referer + start, len);
    tmp[len] = '\0';
    char *dir = dirname(tmp);
    free(tmp);
    
    len = strlen(dir);
    size_t len2 = strlen(path);
    char *str = (char *) malloc(len + len2 + 2);
    memcpy(str, dir, len);
    str[len] = '/';
    memcpy(str + len + 1, path, len2);
    str[len + len2 + 1] = '\0';
    free(dir);
    
    int count;
    str_split(str, '/', &count);
    char **stack = (char **) malloc(sizeof(char *) * count);
    
    
    
    
    
    
    /*int *stack = (int *) malloc(sizeof(int) * count);
    int index = 0;
    size_t total_len = 0;
    char *prev = str;
    char *p = str;
    for(int i = 0; i < count; i++) {
        len = strlen(p);
        if(strcmp(p, "..")) {
            stack[index++] = i;
            total_len += len + 1;
        } else {
            index -= 1;
            total_len -= strlen(prev) + 1;
        }
        prev = p;
        p += len + 1;
    }
    total_len -= 1;
    
    char *result = (char *) malloc(total_len + 1);
    result[total_len] = '\0';
    for(int i = 0, j = 0, k = 0; i < index; i++) {
        p = str;
        for(j = 0; j < stack[i]; j++) {
            p += strlen(p) + 1;
        }
        len = strlen(p);
        memcpy(result + k, p, len);
        k += len;
        result[k++] = '/';
    }*/
    free(stack);
    free(str);
    return result;
}

int main(int argc, char **argv) {
    char *str = normalize_path("../../http/http.js", "https://ryze.sh/lib/std/fs/1.0.0/fs.js");
    printf("str: %s\n", str);
    free(str);
    return 0;
}
