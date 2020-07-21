server: main.cpp
		g++ -o server main.cpp ./ioProcessUnit/http_conn.cpp ./sqlConn/sqlConnPool.cpp -lpthread
clean:
		rm -r server

