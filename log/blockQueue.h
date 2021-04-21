/*
线程安全的队列，利用生产者消费者模型构建
主要接口是push()和pop()
push():生产者操作：队列满了则无法加入新的元素，没满则可以添加元素，同时要唤醒所有等待的线程
pop()：消费者操作：空队列则睡眠等待生产则唤醒
*/
#ifndef __BLOCKQUEUE_H__
#define __BLOCKQUEUE_H__
#include "../lock/locker.h"
#include <string>
template <typename T>
class blockQueue{
public:
    blockQueue(int max_size = 1000){ //构造函数
        if(max_size < 0){
            exit(-1);
        }
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    ~blockQueue(){  // 析构
        m_mutex.lock();
        if(m_array != NULL){
            delete [] m_array;
        }
        m_mutex.unlock();
    }
    void clear(){   // 清除队列相关参 数
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }
    bool full(){    // 判断队列是否满了
        m_mutex.lock();
        if(m_size >= m_max_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool empty(){  // 判断队列是否为空
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool front(T &value){ // 返回队首元素
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    bool back(T &value){ // 返回队尾元素
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }
    int size(){//返回队列大小
        int ret;
        m_mutex.lock();
        ret = m_size;
        m_mutex.unlock();
        return ret;
    } 
    int max_size(){ // 返回队列最大容量
        int ret = 0;
        m_mutex.lock();
        ret = m_max_size;
        m_mutex.unlock();
        return ret;
    }
    bool push(const T &item){ // 往队首添加元素
        m_mutex.lock();
        if(m_size >= m_max_size){
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1)%m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    bool pop(T &item){ // 从队尾取出元素
        m_mutex.lock();
        while(m_size == 0){
            if(!m_cond.wait(m_mutex.getLock())){ // 队列为空则要睡眠
                m_mutex.unlock();
                return false;
            }
        }
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }
private:
    locker m_mutex;
    cond m_cond;
    T *m_array;  // 队列 使用循环数组模拟
    int m_size;  // 当前队列大小
    int m_max_size; // 队列最大容量
    int m_front; // 队首
    int m_back; // 对尾部

};

#endif