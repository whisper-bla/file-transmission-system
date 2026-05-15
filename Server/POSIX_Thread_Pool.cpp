#include "POSIX_Thread_Pool.h"
#include <unistd.h>

// 线程入口函数
void *ptp_start_routine(void *arg)
{
    // 获取线程池
    ptp_t *thread_pool = (ptp_t *)arg;

    // 循环执行任务和等待任务
    do
    {
        // 上锁，创建临界区
        pthread_mutex_lock(&thread_pool->thread_mutex);

        // 进入休眠：等待任务条件产生
        pthread_cond_wait(&thread_pool->thread_cond, &thread_pool->thread_mutex);

        // 如果返回了，说明有任务了，判断任务链表是否为空
        if (!thread_pool->thread_tasks)
        {
            // 判断线程池是否处于开启状态
            if (!thread_pool->thread_status)
                break;
            pthread_mutex_unlock(&thread_pool->thread_mutex);
            continue; // 没抢到任务，继续休眠
        }

        // 提取任务节点，并执行任务
        task_t *task_node = thread_pool->thread_tasks; // 获取任务链表的第一个任务节点

        // 更新任务节点
        thread_pool->thread_tasks = thread_pool->thread_tasks->next;

        // 解锁
        pthread_mutex_unlock(&thread_pool->thread_mutex);

        // 执行任务
        (task_node->task_point)(task_node->args); // 等价执行task_point指针指向的那个函数

        // 执行完毕了之后，销毁任务节点
        task_node->next;
        delete task_node;

        // 判断线程池是否处于开启状态
        pthread_mutex_lock(&thread_pool->thread_mutex);
        if (!thread_pool->thread_status)
            break;
        pthread_mutex_unlock(&thread_pool->thread_mutex);

    } while (1);
    pthread_mutex_unlock(&thread_pool->thread_mutex);
    return nullptr;
}

// 轮询线程函数
void *loop_task(void *args)
{
    // 就是一直判断有没有任务，有任务就唤醒线程执行。
    ptp_t *thread_pool = (ptp_t *)args;

    while (thread_pool->thread_status)
    {
        // 上锁
        pthread_mutex_lock(&thread_pool->thread_mutex);
        if (thread_pool->thread_tasks)
            pthread_cond_signal(&thread_pool->thread_cond);
        pthread_mutex_unlock(&thread_pool->thread_mutex);

        // 延时一段时间，降低CPU的占用率，相当于每隔多少时间检查一下
        usleep(1000);
    }
    return nullptr;
}

ptp_t *ptp_init(int count)
{
    // 申请线程池的空间
    ptp_t *thread_pool = new ptp_t;

    /*
        对线程池的成员变量进行初始化
            1、线程池的线程个数
            2、通过线程池中线程个数，为线程集合申请线程ID集合空间
            3、设置线程池状态
            4、任务列表初始化
            5、初始化线程互斥锁
            6、初始化条件变量
            7、创建线程池中的线程
            8、创建任务轮询线程
    */

    // 初始化线程池的线程个数
    thread_pool->thread_count = count;

    // 通过线程池中线程个数，为线程集合申请线程ID集合空间
    thread_pool->thread_id = new pthread_t[count];

    // 设置线程池的初始状态
    thread_pool->thread_status = true;

    // 任务列表初始化
    thread_pool->thread_tasks = nullptr;

    // 初始化线程互斥锁
    pthread_mutex_init(&thread_pool->thread_mutex, nullptr);

    // 初始化线程条件变量
    pthread_cond_init(&thread_pool->thread_cond, nullptr);

    // 为线程池创建count个线程待命
    for (int i = 0; i < count; i++)
        // 判断线程是否创建成功
        if (pthread_create(thread_pool->thread_id + i, nullptr, ptp_start_routine, thread_pool) != 0)
            i--; // 抵消循环的i++操作

    // 创建一个轮询线程，不断轮询任务链表
    while (pthread_create(&thread_pool->loop_task, nullptr, loop_task, thread_pool))
        ;

    // 返回创建好的线程池指针
    return thread_pool;
}

void ptp_destroy(ptp_t *&thread_pool)
{
    // 关闭线程池
    pthread_mutex_lock(&thread_pool->thread_mutex);
    thread_pool->thread_status = false; // 所有线程都可以访问“状态”
    pthread_mutex_unlock(&thread_pool->thread_mutex);

    // 任务轮询线程进行回收
    pthread_join(thread_pool->loop_task, nullptr);

    // 销毁任务链表
    task_t *task_ptr = thread_pool->thread_tasks;
    while (task_ptr) // 删除链表
    {
        thread_pool->thread_tasks = thread_pool->thread_tasks->next;
        task_ptr->args = nullptr;
        task_ptr->task_point = nullptr;
        task_ptr->next = nullptr;
        delete task_ptr;
        task_ptr = thread_pool->thread_tasks;
    }

    // 唤醒所有的线程
    pthread_cond_broadcast(&thread_pool->thread_cond);

    // 回收所有线程
    for (int i = 0; i < thread_pool->thread_count; i++)
        pthread_join(thread_pool->thread_id[i], nullptr);

    // 销毁线程ID集的空间
    delete[] thread_pool->thread_id;

    // 销毁互斥锁
    pthread_mutex_destroy(&thread_pool->thread_mutex);

    // 销毁条件变量
    pthread_cond_destroy(&thread_pool->thread_cond);

    // 将线程池的空间进行释放
    delete thread_pool;
    thread_pool = nullptr;
}

void ptp_add_task(ptp_t *thread_pool, task_point_t task, void *args)
{
    /*
        任务是存储在一个任务节点中，任务节点是在一个任务链表中
            所以新增任务，其实就是为链表新增一个节点
    */
    task_t *task_node = new task_t;

    // 初始化任务节点
    task_node->task_point = task;
    task_node->args = args;
    task_node->next = nullptr;

    // 将新节点尾插到任务链表中
    // 因为任务链表对于所有线程而言是一个共享的公共资源，所以要保证它是被有序访问。
    pthread_mutex_lock(&thread_pool->thread_mutex);
    if (thread_pool->thread_tasks == nullptr)
        thread_pool->thread_tasks = task_node;
    else
    {
        // 找到最后一个节点
        task_t *task_tail = thread_pool->thread_tasks;
        while (task_tail->next)
            task_tail = task_tail->next;
        task_tail->next = task_node;
    }
    pthread_mutex_unlock(&thread_pool->thread_mutex);
}

bool ptp_task_is_null(ptp_t *thread__pool)
{
    if (thread__pool == nullptr)
        return false;
    else if (thread__pool->thread_tasks == nullptr)
        return true;
    return false;
}