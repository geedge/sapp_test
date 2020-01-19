#ifndef __HASH_ARRAY_H__
#define __HASH_ARRAY_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dns_hash_table.h"

struct HashTableArrayNode
{
	struct HashTable *hash_table;
};

struct HashTableArray
{
	struct HashTableArrayNode *nodes;
	int nnodes;
	int size;
};

struct HashTableArray * hash_table_array_new (int size);

struct HashTable* hash_table_array_node_new (HashFunc hash_func,CompareFunc key_compare_funcs);

void hash_table_array_delete_node (struct HashTableArray *hash_table_array, int location);

void hash_table_array_insert_node (struct HashTableArray *hash_table_array,int location,HashFunc hash_func,CompareFunc key_compare_func);

void hash_table_array_destory(struct HashTableArray *hash_table_array,HFunc hfunc);

int hash_table_array_init(struct HashTableArray * hash_table_array,HashFunc hash_func,CompareFunc key_compare_funcs);

void hash_table_array_print(struct HashTableArray * hash_table_array,HFunc hfunc);


#endif
