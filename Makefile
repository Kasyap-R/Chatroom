all:
	gcc --debug -o web_server src/main.c src/listener.c src/domain_sock.c
