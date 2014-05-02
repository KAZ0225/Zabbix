kaz: kaz.c
	gcc -shared -fPIC \
		-W -Wall -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith \
		-o kaz.so kaz.c \
		-I../../../include \
		-L/usr/lib64/mysql \
		-lmysqlclient
