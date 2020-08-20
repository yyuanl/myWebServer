#ifndef __SQLCONN__
#define __SQLCONN__
#include <string>
#include <mysql/mysql.h>
#include <list>
#include <semaphore.h>
#include "../lock/locker.h"

using namespace std;

class sqlConnPool{
public:
    //单例模式（懒汉）,外部唯一的接口
    static sqlConnPool *getInstance(string m_ip, string m_user, string m_passWord, string m_dataBastName, int m_port ,unsigned int m_maxConn);
    //数据库连接池外部接口
    MYSQL *getOneConn(); //从空闲连接获得一个连接
    bool realseUsedConn(MYSQL *conn); // 释放使用的连接
    int getFreeConnNumb(); // 获得空闲连接数
private:
    //构造  拷贝构造  复制构造 保证单例模式
    sqlConnPool(const sqlConnPool &conn){}
    sqlConnPool(){}
    sqlConnPool& operator = (const sqlConnPool &conn){}
    sqlConnPool(std::string m_ip, std::string m_user, std::string m_passWord, std::string m_dataBastName, int m_port ,unsigned int m_maxConn);
    void destroyPool(); //close每一个连接资源
    ~sqlConnPool(){destroyPool();} //释放池中所有连接    
// 池属性
private:
    unsigned int maxConnNum;
    unsigned int usedConnNum;
    unsigned int freeConnNum;
//线程安全
private:
    list<MYSQL *>connList;
    locker lock;
    sem m_sem;
// 数据库
private:
    string ip;
    string userNameMysql;  // 登录数据库用户名
    string passWord; 
    string dataBaseName;
    string port;
};

class sqlConRALL{
public:
    sqlConRALL(MYSQL *&sql_obj, sqlConnPool *sql_conn_pool){  //用户获得的sql连接，用RALL类包装，对象生命周期结束，连接（资源）也自动释放
        sql_obj = sql_conn_pool->getOneConn();
        one_conn = sql_obj;
        pool=sql_conn_pool;
    }
    ~sqlConRALL(){
        pool->realseUsedConn(one_conn);
    }
private:
    MYSQL *one_conn;
    sqlConnPool *pool;
};








   



#endif