server: main.cpp
		g++ -o server main.cpp ./ioProcessUnit/http_conn.cpp -lpthread
clean:
		rm -r server

