server: main.cpp
		g++ -g -o server main.cpp ./httpConnection/http_conn.cpp ./sqlConn/sqlConnPool.cpp ./utils/utils.cpp ./log/log.cpp ./log/blockQueue.h -lpthread -I/usr/include/mysql -lmysqlclient -Wno-format-truncation
clean:
		rm -r server

