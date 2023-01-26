all:
	gcc node.c -Wno-deprecated-declarations -lpthread -lcrypto -o node