#include<stdio.h>
#include "dns_heap_sort.h"


static void heapsort_element_swap(void**a, void **b)
{
		void *temp = *a;
		*a = *b;
		*b = temp;
}


static void heapsort_array_sort(void **arr, int index, int num,HeapSortCmpFunc HCFunc)
{
    int child;
	void *tmp = NULL;

    for (tmp =  arr[index]; 2*index + 1 < num; index = child){
        child = 2*index + 1; 
        if (child != num - 1 && (*HCFunc)(arr[child+ 1],arr[child], HEAP_SORT_LESS_TAG))
            ++child;     
        if ((*HCFunc)(tmp,arr[child],HEAP_SORT_GREATER_TAG))
            arr[index] = arr[child];
        else
            break;
    }
    arr[index] = tmp;
}


struct HeapSortArray * heapsort_array_new(HeapSortInitNodeFunc HIFunc,HeapSortCmpFunc HCFunc, int size)
{
	struct HeapSortArray *heap_sort_array = (struct HeapSortArray *)malloc(sizeof(struct HeapSortArray));
	memset(heap_sort_array,0,sizeof(struct HeapSortArray));
	heap_sort_array->HIFunc = HIFunc;
	heap_sort_array->HCFunc = HCFunc;
	heap_sort_array->size = size;
	heap_sort_array->nnodes = (void**)calloc(sizeof(void **), size);
	return heap_sort_array;
}


int  heapsort_array_insert_node(struct HeapSortArray *heap_sort_array,int len_node,void *userdata)
{
	if(!heap_sort_array)
		return 1;

	heap_sort_array->nnodes[heap_sort_array->used_size] = malloc(len_node);
	memset(heap_sort_array->nnodes[heap_sort_array->used_size],0,len_node);	
	(*heap_sort_array->HIFunc)(heap_sort_array->nnodes[heap_sort_array->used_size],userdata);
	heap_sort_array->used_size ++;
	return 0;
}


void heapsort_to_do_sort(struct HeapSortArray *heap_sort_array, int size)
{
	int i;
	if(!heap_sort_array->nnodes)
		return;
	for (i = size / 2; i >= 0; --i)
		heapsort_array_sort(heap_sort_array->nnodes, i, size,heap_sort_array->HCFunc);
	for(i=size -1;i>0;--i)
	{
	    heapsort_element_swap((void **)heap_sort_array->nnodes,(void **)heap_sort_array->nnodes + i); 
	    heapsort_array_sort(heap_sort_array->nnodes, 0, i, heap_sort_array->HCFunc);
	}
}


void heapsort_destory(struct HeapSortArray *heap_sort_array)
{
	int i;
	if(!heap_sort_array->nnodes)
		return;
	for(i = 0; i < heap_sort_array->size; i ++)
	{
		if(heap_sort_array->nnodes[i]){
			free(heap_sort_array->nnodes[i]);
			heap_sort_array->nnodes[i] = NULL;
		}
	}
	free(heap_sort_array->nnodes);
	free(heap_sort_array);
}


void heapsort_print(struct HeapSortArray *heap_sort_array, int size, HSFunc func_print)
{
	int i;
	if(!heap_sort_array->nnodes)
		return;
	
	for(i = 0; i < size; i ++)
	{
		(*func_print)(heap_sort_array->nnodes[i]);
	}
}


