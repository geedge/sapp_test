#include "dns_hash_table.h"
#include "dns_hash_array.h"



struct HashTableArray * hash_table_array_new (int size)
{
	struct HashTableArray *hash_table_array;
	hash_table_array = (struct HashTableArray *)calloc(sizeof(struct HashTableArray),1);
	hash_table_array->size = size;
	hash_table_array->nodes = (struct HashTableArrayNode *)calloc((size_t)sizeof(struct HashTableArrayNode),(size_t)size);
	return hash_table_array;
}

int hash_table_array_init(struct HashTableArray * hash_table_array,HashFunc hash_func,CompareFunc key_compare_funcs)
{
	int i = 0;

	if(!hash_table_array)
		return 1;
	
	for(i = 0; i < hash_table_array->size; i ++)
	{
		hash_table_array_insert_node(hash_table_array, i, hash_func,key_compare_funcs);
	}
	return 0;
}


struct HashTable* hash_table_array_node_new (HashFunc hash_func,CompareFunc key_compare_funcs)
{
	if(!hash_func || !key_compare_funcs)
		return NULL;
	return hash_table_new (hash_func,key_compare_funcs);
}



void hash_table_array_delete_node (struct HashTableArray *hash_table_array, int location)
{
	if(!hash_table_array)
  		return;
	
	if(hash_table_array->nodes[location].hash_table)
	{
		hash_table_destroy((hash_table_array->nodes[location]).hash_table);
		hash_table_array->nodes[location].hash_table = NULL;	
		hash_table_array->nnodes --;
	}
		
}


void hash_table_array_insert_node (struct HashTableArray *hash_table_array,int location,HashFunc hash_func,CompareFunc key_compare_func)
{
	if(!hash_table_array)
  		return;
	if((hash_table_array->nodes[location]).hash_table)
	{
		hash_table_array_delete_node(hash_table_array,location);
	}
	(hash_table_array->nodes[location]).hash_table = hash_table_array_node_new(hash_func,key_compare_func); 
	hash_table_array->nnodes ++;
}


void hash_table_array_destory(struct HashTableArray *hash_table_array,HFunc hfunc)
{
	if(!hash_table_array)
		return;
	int i;
	for(i = 0; i < hash_table_array->size; i ++)
	{
		if(hash_table_array->nodes[i].hash_table){
			if(hfunc)
				hash_table_foreach(hash_table_array->nodes[i].hash_table,hfunc,NULL);
			hash_table_destroy((hash_table_array->nodes[i]).hash_table);
			hash_table_array->nodes[i].hash_table = NULL;
		}
	}
	free(hash_table_array->nodes);
	free(hash_table_array);
}



void hash_table_array_print(struct HashTableArray * hash_table_array,HFunc hfunc)
{
	int i;
	if(!hash_table_array)
		return;
	for(i = 0; i < hash_table_array->nnodes;i ++)
	{
		hash_table_foreach(hash_table_array->nodes[i].hash_table, hfunc, NULL);	
	}

}



