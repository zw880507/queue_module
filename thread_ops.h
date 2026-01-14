#pragma once
typedef struct thread_ops {
    void (*get_thread_name)(char **thread_name);
    void (*get_thread_count)(int *thread_count);
    void (*get_thread_priority)(int *thread_priority);

} thread_ops_t;

extern const thread_ops_t default_thread_ops;
