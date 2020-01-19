all:
	gcc -Wall -g -fPIC -shared -I./include/ ./dns_topn.c ./dns_hash_table.c ./dns_hash_array.c ./dns_heap_sort.c  -o dns_topn.so
clean:
	rm -rf *.o *.so
