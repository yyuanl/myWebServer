server: main.cpp
		g++ -g -o server main.cpp ./ioProcessUnit/http_conn.cpp ./sqlConn/sqlConnPool.cpp -lpthread -I/usr/include/mysql -lmysqlclient 
clean:
		rm -r server