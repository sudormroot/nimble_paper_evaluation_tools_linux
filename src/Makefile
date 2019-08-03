
CC=gcc


memory_access_time: memory_access_time.c
	$(CC) -o $@ $^ -lnuma
	sudo setcap "all=ep" $@
