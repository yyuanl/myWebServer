#include "sqlConnPool.h"
#include <stdio.h>
#include <iostream>
using namespace std;
// #define NDEBUG


/* 外部接口*/
sqlConnPool *sqlConnPool::getInstance(std::string m_ip, std::string m_user, std::string m_passWord, std::string m_dataBastName, int m_port ,unsigned int m_maxConn){
    
#ifndef NDEBUG
std::cout<<"call sqlConnPool::getInstance"<<std::endl;
#endif  
    static sqlConnPool uniquePoolInstance(m_ip,m_user,m_passWord,m_dataBastName, m_port ,m_maxConn);  // 线程安全
    return &uniquePoolInstance;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *sqlConnPool::getOneConn(){
    MYSQL *con = NULL;
	if (0 == connList.size())
		return NULL;
	m_sem.p();
	lock.lock();
	con = connList.front();
	connList.pop_front();
	--freeConnNum;
	++usedConnNum;
	lock.unlock();
	return con;
}
bool sqlConnPool::realseUsedConn(MYSQL *conn){
    if (NULL == conn)
		return false;

	lock.lock();

	connList.push_back(conn);
	++freeConnNum;
	--usedConnNum;

	lock.unlock();
	m_sem.v();
	return true;
}
int sqlConnPool::getFreeConnNumb(){
    return freeConnNum;
}
/*  私有成员函数*/

sqlConnPool::sqlConnPool(std::string m_ip, std::string m_userNameMysql, std::string m_passWord, std::string m_dataBaseName, int m_port ,unsigned int m_maxConn)
                    :usedConnNum(0),freeConnNum(0),maxConnNum(m_maxConn), 
                    ip(m_ip),
                    userNameMysql(m_userNameMysql),
                    passWord(m_passWord),
                    dataBaseName(m_dataBaseName),
                    port(to_string(m_port))
                    
{

    lock.lock();
	for (int i = 0; i < maxConnNum; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);
		if (con == NULL)
		{
  
			cout << "Error:" << mysql_error(con);
			exit(1);
		}
#ifndef NDEBUG
std::cout<<"check args"<<std::endl;
printf("ip is %s\n", ip.c_str());
printf("userName is %s\n", userNameMysql.c_str());
printf("passWord is %s\n", passWord.c_str());
printf("dataBaseName is %s\n", dataBaseName.c_str());
printf("m_port is %d\n", m_port);
#endif

		con = mysql_real_connect(con, ip.c_str(), userNameMysql.c_str(), passWord.c_str(), dataBaseName.c_str(), 0, NULL, 0);
		if (con == NULL)
		{
			cout << "Error: " << mysql_error(con)<<endl;
            cout << "Error: " << mysql_errno(con)<<endl;
#ifndef NDEBUG
std::cout<<"mysql_real_connect error"<<std::endl;
#endif    
			exit(1);
		}
		connList.push_back(con);
		++freeConnNum;
	}
	m_sem = sem(freeConnNum);
	maxConnNum = freeConnNum;
	lock.unlock();
}

void sqlConnPool::destroyPool(){
    lock.lock();
	if (connList.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		usedConnNum = 0;
		freeConnNum = 0;
		connList.clear();
		//lock.unlock();
	}
	lock.unlock();
}
