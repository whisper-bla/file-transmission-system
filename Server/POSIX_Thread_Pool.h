#ifndef _POSIX_THREAD_POOL_H__
#define _POSIX_THREAD_POOL_H__

#include <pthread.h>

/*
    POSIX_Thread_Pool:基于pthread线程库开发的线程池，简称：PTP
        该库为学习使用，免费开源，禁止商用。

        数据类型：
            task_point_t :任务函数指针类型，用于表示一个任务函数的。
            task_t       :任务节点类型，存储任务函数指针和任务函数的参数集。
            ptp_t        :线程池类型，用于描述一个线程池。

        函数接口：
            ptp_t *ptp_init(int count);
                @描述：
                    创建一个线程池并初始化
                    初始化：ptp_t 成员变量
                    - `thread_count`：线程池中最大线程数量，即能够支持的最高并发数目
                    - `thread_status`：线程池当前状态，启动或停止状态
                    - `thread_id`：线程池中服役线程集合
                    - `thread_mutex`：线程池中线程的共享互斥锁
                    - `thread_cond`：线程池中线程共享的条件变量
                    - `thread_tasks`：线程池中线程所需要执行的任务链表
                    - `loop_task`：线程池中轮询线程池任务链表的线程
                    @count:
                    设置线程池中最大服役/并发线程数量
                    @return：
                    成功返回创建并初始化完毕的线程池指针
                    失败返回NULL

            void ptp_destroy(ptp_t *&thread_pool);
                @描述：
                    销毁一个已经存在线程池
                    @thread_pool:
                    需要销毁的线程池指针引用

            void ptp_add_task(ptp_t *thread_pool,task_point_t task,void *args);
                @描述：
                    往一个已存在的线程池中增加任务
                    @thread_pool：
                    需要增加任务的线程池指针
                    @task：
                    增加的任务函数指针
                    @args：
                    任务函数执行过程中需要的参数

            bool ptp_task_is_null(ptp_t *thread__pool);
                @描述
                    用于判断指定的线程池任务链表是否为空
                    @thread_pool:
                    需要判断任务链表是否为空的线程池指针
                    @return：
                    不为空返回false，为空返回true


*/

typedef void (*task_point_t)(void *);
// 任务结构类型
typedef struct tasks
{
    // 任务指针
    task_point_t task_point;
    /*
        // 任务函数需要符合这个规则
        void task(void *data)
        {
        // 需要执行的任务
        }
    */

    // 任务执行所需要参数
    void *args;

    // 下一个任务
    struct tasks *next;
} task_t;

typedef struct posix_thread_pool
{
    // 线程的个数
    int thread_count;

    // 线程池状态
    bool thread_status;

    // 线程集合
    pthread_t *thread_id;

    // 线程池中线程共享的互斥锁
    pthread_mutex_t thread_mutex;

    // 线程池中线程共享的条件变量，即通知
    pthread_cond_t thread_cond;

    // 线程任务链表
    task_t *thread_tasks;

    // 轮询任务的线程
    pthread_t loop_task;
    /*
        最大线程的数目：表示可以支持线程并发的最大线程数
        当前服役的线程数目：表示当前能够并发的线程数量
        当前休眠的线程数目：表示当前正在待命且可以执行任务的线程数量。
        ...
    */
} ptp_t;

ptp_t *ptp_init(int count);
/*
    @描述：
        创建一个线程池并初始化
        初始化：ptp_t 成员变量
        - `thread_count`：线程池中最大线程数量，即能够支持的最高并发数目
        - `thread_status`：线程池当前状态，启动或停止状态
        - `thread_id`：线程池中服役线程集合
        - `thread_mutex`：线程池中线程的共享互斥锁
        - `thread_cond`：线程池中线程共享的条件变量
        - `thread_tasks`：线程池中线程所需要执行的任务链表
        - `loop_task`：线程池中轮询线程池任务链表的线程
        @count:
        设置线程池中最大服役/并发线程数量
        @return：
        成功返回创建并初始化完毕的线程池指针
        失败返回NULL
*/

void ptp_destroy(ptp_t *&thread_pool);
/*
    @描述：
        销毁一个已经存在线程池
        @thread_pool:
        需要销毁的线程池指针引用
*/

void ptp_add_task(ptp_t *thread_pool, task_point_t task, void *args);
/*
    @描述：
        往一个已存在的线程池中增加任务
        @thread_pool：
        需要增加任务的线程池指针
        @task：
        增加的任务函数指针
        @args：
        任务函数执行过程中需要的参数
*/

bool ptp_task_is_null(ptp_t *thread__pool);
/*
    @描述
        用于判断指定的线程池任务链表是否为空
        @thread_pool:
        需要判断任务链表是否为空的线程池指针
        @return：
        不为空返回false，为空返回true
*/

#endif //_POSIX_THREAD_POOL_H__