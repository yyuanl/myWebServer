// #include "blockQueue.h"
// #include <stdlib.h>
// template <class T>
// blockQueue<T>::blockQueue(int max_size){
//     if(max_size < 0){
//         exit(-1);
//     }
//     m_max_size = max_size;
//     m_array = new T[max_size];
//     m_size = 0;
//     m_front = -1;
//     m_back = -1;
// }

// template <class T>
// blockQueue<T>::~blockQueue(){
//     m_mutex.lock();
//     if(m_array != NULL){
//         delete [] m_array;
//     }
//     m_mutex.unlock();
// }

// template <class T>
// void blockQueue<T>::clear(){
//     m_mutex.lock();
//     m_size = 0;
//     m_front = -1;
//     m_back = -1;
//     m_mutex.unlock();
// }

// template <class T>
// bool blockQueue<T>::full(){
//     m_mutex.lock();
//     if(size >= m_max_size){
//         m_mutex.unlock();
//         return true;
//     }
//     m_mutex.unlock();
//     return false;
// }

// template <class T>
// bool blockQueue<T>::empty(){
//     m_mutex.lock();
//     if(m_size == 0){
//         m_mutex.unlock();
//         return true;
//     }
//     m_mutex.unlock();
//     return false;
// }

// template <class T>
// bool blockQueue<T>::front(T &value){
//     m_mutex.lock();
//     if(m_size == 0){
//         m_mutex.unlock();
//         return false;
//     }
//     value = m_array[m_front];
//     m_mutex.unlock();
//     return true;
// }

// template <class T>
// bool blockQueue<T>::back(T &value){
//     m_mutex.lock();
//     if(m_size == 0){
//         m_mutex.unlock();
//         return false;
//     }
//     value = m_array[m_back];
//     m_mutex.unlock();
//     return true;
// }

// template <class T>
// int blockQueue<T>::size(){
//     int ret;
//     m_mutex.lock();
//     ret = m_size;
//     m_mutex.unlock();
//     return ret;
// }

// template <class T>
// int blockQueue<T>::max_size(){
//     int ret = 0;
//     m_mutex.lock();
//     ret = m_max_size;
//     m_mutex.unlock();
//     return ret;
// }

// template <class T>
// bool blockQueue<T>::push(const T &item){
//     m_mutex.lock();
//     if(m_size >= m_max_size){
//         m_cond.broadcast();
//         m_mutex.unlock();
//         return false;
//     }
//     m_back = (m_back + 1)%m_max_size;
//     m_array[m_back] = item;
//     m_size++;
//     m_cond.broadcast();
//     m_mutex.unlock();
//     return true;
// }

// template <class T>
// bool blockQueue<T>::pop(T &item){
//     m_mutex.lock();
//     while(m_size == 0){
//         if(!m_cond.wait(m_mutex.getLock())){ // 队列为空则要睡眠
//             m_mutex.unlock();
//             return false;
//         }
//     }
//     m_front = (m_front + 1) % m_max_size;
//     item = m_array[m_front];
//     m_size--;
//     m_mutex.unlock();
//     return true;
// }
