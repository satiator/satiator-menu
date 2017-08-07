CFLAGS := -ggdb3 -Wall -DTEST

default: image2desc dumpdesc

image2desc: cdparse.o cue2desc.o test_image2desc.o
	$(CC) $^ -o $@

dumpdesc: dumpdesc.o

clean:
	rm -f image2desc dumpdesc *.o
