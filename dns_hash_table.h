#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163

#undef	CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


typedef int (*HashFunc)		(const void *key);

typedef int	(*CompareFunc)		(const void *a,const void *b);

typedef int	(*HRFunc)	 (void *key,void *value,void *user_data);

typedef void (*HFunc)	(void *	key,void *value,void *user_data);

#if 0
struct HashNode
{
  void * key;
  void * value;
  struct HashNode *next;
};
#endif


struct HashNode
{
  void *key;
  void *value;
  struct HashNode *next;
};



struct HashTable
{
  int size;
  int nnodes;
  unsigned int frozen;
  struct HashNode **nodes;
  HashFunc hash_func;
  CompareFunc key_compare_func;
};


//struct HashTable* hash_table_new (HashFunc      hash_func,CompareFunc key_compare_func);
struct HashTable* hash_table_new (HashFunc        hash_func,CompareFunc key_compare_func);


void hash_table_destroy (struct HashTable *hash_table);

void *hash_table_lookup (struct HashTable	 *hash_table,	  const void * key);

void hash_table_insert (struct HashTable *hash_table,void *key,void *value);

void hash_table_remove (struct HashTable	     *hash_table,const void *    key);

int hash_table_lookup_extended (struct HashTable	*hash_table,const void *	 lookup_key,void * *orig_key,void *		*value);

void hash_table_freeze (struct HashTable *hash_table);

void hash_table_thaw (struct HashTable *hash_table);

int hash_table_foreach_remove (struct HashTable	*hash_table,HRFunc	 func,void *	 user_data);

void hash_table_foreach (struct HashTable *hash_table,HFunc	func,void *user_data);

int hash_table_size (struct HashTable *hash_table);


#endif
