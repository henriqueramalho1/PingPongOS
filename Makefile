all:
	gcc -o ppos-test pingpong-disco2.c ppos-core-aux.c ppos_disk.c disk.c libppos_static.a -lrt

clean:
	rm -f ppos-test