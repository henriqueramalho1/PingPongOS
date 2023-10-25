all:
	gcc -o ppos-test ppos-core-aux.c pingpong-scheduler-srtf.c libppos_static.a

clean:
	rm -f ppos-test