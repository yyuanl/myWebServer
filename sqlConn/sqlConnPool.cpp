#include "sqlConnPool.h"


/* 外部接口*/
sqlConnPool *sqlConnPool::getInstance(std::string m_ip, std::string m_user, std::string m_passWord, std::string m_dataBastName, int m_port ,unsigned int m_maxConn){
    static sqlConnPool uniquePoolInstance(m_ip,m_user,m_passWord,m_dataBastName, m_port ,m_maxConn);  // 线程安全
    return &uniquePoolInstance;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *sqlConnPool::getOneConn(){
    MYSQL *con = NULL;
	if (0 == connPool.size())
		return NULL;
	m_sem.p();
	lock.lock();
	con = connPool.front();
	connPool.pop_front();
	--freeConnNum;
	++usedConnNum;
	lock.unlock();
	return con;
}
bool sqlConnPool::realseUsedConn(MYSQL *conn){
    if (NULL == conn)
		return false;

	lock.lock();

	connPool.push_back(con);
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

sqlConnPool::sqlConnPool(std::string m_ip, std::string m_userName, std::string m_passWord, std::string m_dataBastName, int m_port ,unsigned int m_maxConn)
                    :usedConnNum(0),freeConnNum(0),ip(m_ip),userName(m_userName),passWord(m_passWord),dataBastName(m_dataBastName),
                    port(m_port), maxConnNum(m_maxConn){
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
		con = mysql_real_connect(con, ip.c_str(), userName.c_str(), passWord.c_str(), dataBastName.c_str(), port, NULL, 0);
		if (con == NULL)
		{
			cout << "Error: " << mysql_error(con);
			exit(1);
		}
		connPool.push_back(con);
		++freeConnNum;
	}
	m_sem = sem(freeConnNum);
	maxConnNum = freeConnNum;
	lock.unlock();
}

void sqlConnPool::destroyPool(){
    lock.lock();
	if (connPool.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		userName = 0;
		freeConnNum = 0;
		connPool.clear();
		//lock.unlock();
	}
	lock.unlock();
}
