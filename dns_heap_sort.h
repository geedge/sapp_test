#ifndef __HEAP_SORT_H__
#define __HEAP_SORT_H__

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define HEAP_SORT_EQUAL_TAG 0
#define HEAP_SORT_LESS_TAG 1
#define HEAP_SORT_GREATER_TAG 2

typedef int (*HeapSortInitNodeFunc)		(void *node,void *userdata);
typedef int (*HeapSortCmpFunc)		(void *a,void *b, int type);
typedef void (*HSFunc)		(void *data);


struct HeapSortArray
{
		void **nnodes;
		int size;
		int used_size;
		HeapSortInitNodeFunc HIFunc;
		HeapSortCmpFunc HCFunc;
};



struct HeapSortArray * heapsort_array_new(HeapSortInitNodeFunc HIFunc,HeapSortCmpFunc HCFunc, int size);

int  heapsort_array_insert_node(struct HeapSortArray *heap_sort_array,int len_node,void *userdata);

void heapsort_to_do_sort(struct HeapSortArray *heap_sort_array, int size);

void heapsort_destory(struct HeapSortArray *heap_sort_array);

void heapsort_print(struct HeapSortArray *heap_sort_array, int size, HSFunc func_print);



#endif
