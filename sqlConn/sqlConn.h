#ifndef __SQLCONN__
#define __SQLCONN__
#include <string>
#include <mysql/mysql.h>
#include <list>
#include <semaphore.h>
#include "../lock/locker.h"


class sqlConn{
public:
    //单例模式（懒汉）,外部唯一的接口
    static sqlConn *getInstance(string m_ip, string m_user, string m_passWord, string m_dataBastName, int m_port ,unsigned int m_maxConn);
    //数据库连接池外部接口
    MYSQL *getOneConn(); //从空闲连接获得一个连接
    bool realseUsedConn(MYSQL *conn); // 释放使用的连接
    int getFreeConnNumb(); // 获得空闲连接数
private:
    //构造  拷贝构造  复制构造 保证单例模式
    sqlConn(unsigned int m_usedConn = 0, unsigned int m_freeConn = 0, string m_ip, string m_user, string m_passWord, string m_dataBastName, int m_port ,unsigned int m_maxConn);
    sqlConn(const sqlConn &conn){}
    sqlConn& operator = (const sqlConn &conn){}
    void destroyPool();
    ~sqlConn(){destroyPool();} //释放池中所有连接    
// 池属性
private:
    unsigned int maxConnNum;
    unsigned int usedConnNum;
    unsigned int freeConnNum;
//线程安全
private:
    list<MYSQL *>connPool;
    locker lock;
    sem m_sem;
// 数据库
private:
    string m_ip;
    string m_user;
    string m_passWord; 
    string m_dataBastName;
    int m_port;
};
#endif