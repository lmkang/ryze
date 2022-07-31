#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

struct ev_task_info {
    void *(*func)(void *);
    void *arg;
    void (*callback)(void *);
    struct ev_task_info *next;
};

struct ev_thread_info {
    // 0-thread running, 1-pool destroy, 2-thread exit
    int status;
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct ev_task_info *head;
    struct ev_task_info *tail;
    struct ev_thread_info *next;
};

struct ev_thread_pool {
    int min_num;
    int max_num;
    struct ev_thread_info *head;
    struct ev_thread_info *tail;
};

ev_thread_pool *ev_thread_pool_init(int min_num, int max_num);
void ev_thread_pool_destroy(struct ev_thread_pool *pool);
void ev_task_info_add(struct ev_thread_info *thread_info, 
    void *(*func)(void *), void *arg, void (*callback)(void *));
void *ev_task_info_func(void *arg);
char *read_file(const char *file_name);

ev_thread_pool *ev_thread_pool_init(int min_num, int max_num) {
    struct ev_thread_pool *pool = 
        (struct ev_thread_pool *) malloc(sizeof(struct ev_thread_pool));
    pool->min_num = min_num;
    pool->max_num = max_num;
    struct ev_thread_info *prev = NULL;
    struct ev_thread_info *next = NULL;
    for(int i = 0; i < min_num; i++) {
        next = (struct ev_thread_info *) malloc(sizeof(struct ev_thread_info));
        next->status = 0;
        pthread_create(&next->thread, NULL, ev_task_info_func, next);
        pthread_detach(next->thread);
        pthread_cond_init(&next->cond, NULL);
        pthread_mutex_init(&next->mutex, NULL);
        next->head = NULL;
        next->tail = NULL;
        next->next = NULL;
        if(prev == NULL) {
            prev = next;
            pool->head = next;
        } else {
            prev->next = next;
            prev = next;
        }
    }
    pool->tail = next;
    return pool;
}

void ev_thread_pool_destroy(struct ev_thread_pool *pool) {
    struct ev_thread_info *thread_info = pool->head;
    struct ev_thread_info *thread_info_prev;
    struct ev_task_info *task_info;
    struct ev_task_info *task_info_prev;
    while(thread_info != NULL) {
        pthread_mutex_lock(&thread_info->mutex);
        thread_info->status += 1;
        task_info = thread_info->head;
        while(task_info != NULL) {
            task_info_prev = task_info;
            task_info = task_info->next;
            free(task_info_prev);
        }
        thread_info->head = NULL;
        thread_info->tail = NULL;
        thread_info_prev = thread_info;
        thread_info = thread_info->next;
        pthread_mutex_unlock(&thread_info_prev->mutex);
        pthread_cond_signal(&thread_info_prev->cond);
    }
    // waiting task to finish, then release memory
    thread_info = pool->head;
    while(1) {
        pthread_mutex_lock(&thread_info->mutex);
        if(thread_info->status == 2) {
            pthread_cond_destroy(&thread_info->cond);
            pthread_mutex_destroy(&thread_info->mutex);
            thread_info_prev = thread_info;
            thread_info = thread_info->next;
            free(thread_info_prev);
            if(thread_info == NULL) {
                break;
            }
        } else {
            pthread_mutex_unlock(&thread_info->mutex);
            pthread_cond_signal(&thread_info->cond);
        }
    }
}

void ev_task_info_add(struct ev_thread_info *thread_info, 
        void *(*func)(void *), void *arg, void (*callback)(void *)) {
    struct ev_task_info *task_info = 
        (struct ev_task_info *) malloc(sizeof(struct ev_task_info));
    task_info->func = func;
    task_info->callback = callback;
    task_info->arg = arg;
    task_info->next = NULL;
    pthread_mutex_lock(&thread_info->mutex);
    if(thread_info->head == NULL) {
        thread_info->head = task_info;
        thread_info->tail = task_info;
    } else {
        thread_info->tail->next = task_info;
        thread_info->tail = task_info;
    }
    pthread_mutex_unlock(&thread_info->mutex);
    pthread_cond_signal(&thread_info->cond);
    printf("task add: %s\n", (char *) arg);
}

void *ev_task_info_func(void *arg) {
    struct ev_thread_info *thread_info = (struct ev_thread_info *) arg;
    while(1) {
        pthread_mutex_lock(&thread_info->mutex);
        if(thread_info->head == NULL) {
            pthread_cond_wait(&thread_info->cond, &thread_info->mutex);
        }
        if(thread_info->head == NULL) {
            if(thread_info->status == 0) {
                continue;
            } else {
                thread_info->status += 1;
                pthread_mutex_unlock(&thread_info->mutex);
                pthread_cond_signal(&thread_info->cond);
                break;
            }
        }
        struct ev_task_info *task_info = thread_info->head;
        if(thread_info->head == thread_info->tail) {
            thread_info->head = NULL;
            thread_info->tail = NULL;
        } else {
            thread_info->head = thread_info->head->next;
        }
        pthread_mutex_unlock(&thread_info->mutex);
        void *ret = task_info->func(task_info->arg);
        if(task_info->callback != NULL) {
            task_info->callback(task_info->arg);
        }
        // TODO
        free(ret);
        free(task_info);
    }
    printf("task finish\n");
    return NULL;
}

void *_task_func(void *arg) {
    char *file_name = (char *) arg;
    char *ret = read_file(file_name);
    return ret;
}

void _task_callback(void *arg) {
    printf("callback: %s\n", (char *) arg);
}

int main(int argc, char **argv) {
    struct ev_thread_pool *pool = ev_thread_pool_init(4, 8);
    struct ev_thread_info *thread_info = pool->head;
    int len = 4;
    char **strs = (char **) malloc(sizeof(char *) * len);
    for(int i = 0; i < len; i++) {
        strs[i] = (char *) malloc(sizeof(char) * 32);
        sprintf(strs[i], "./test/test%d.txt", i + 1);
    }
    int index = 0;
    int seed2 = 434289031;
    int count = 0;
    while(1) {
        ev_task_info_add(thread_info, 
            _task_func, (void *) strs[index++ % len], _task_callback);
        srand(seed2++);
        sleep(rand() % 5 + 1);
        thread_info = thread_info->next;
        if(thread_info == NULL) {
            thread_info = pool->head;
            if(++count == 16) {
                break;
            }
        }
    }
    sleep(128);
    ev_thread_pool_destroy(pool);
    for(int i = 0; i < len; i++) {
        free(strs[i]);
    }
    free(strs);
    printf("pool destroy\n");
    /*char *str = read_file("./test/test1.txt");
    printf("%s\n", str);
    free(str);*/
    return 0;
}

int seed = 947675424;

char *read_file(const char *file_name) {
    FILE *file = fopen(file_name, "rb");
    if(file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    char *chars = (char *) malloc(sizeof(char) * (size + 1));
    chars[size] = '\0';
    for(size_t i = 0; i < size; ) {
        i += fread(&chars[i], 1, size - i, file);
        if(ferror(file)) {
            fclose(file);
            free(chars);
            return NULL;
        }
    }
    fclose(file);
    srand(seed++);
    sleep(rand() % 4 + 2);
    return chars;
}





























































































































































