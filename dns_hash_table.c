#include<stdio.h>
#include "dns_hash_table.h"



static const unsigned int g_primes[] =
{
  11,
  19,
  37,
  73,
  109,
  163,
  251,
  367,
  557,
  823,
  1237,
  1861,
  2777,
  4177,
  6247,
  9371,
  14057,
  21089,
  31627,
  47431,
  71143,
  106721,
  160073,
  240101,
  360163,
  540217,
  810343,
  1215497,
  1823231,
  2734867,
  4102283,
  6153409,
  9230113,
  13845163,
};

static const unsigned int g_nprimes = sizeof (g_primes) / sizeof (g_primes[0]);

unsigned int
g_spaced_primes_closest (unsigned int num)
{
  int i;

  for (i = 0; i < g_nprimes; i++)
    if (g_primes[i] > num)
      return g_primes[i];

  return g_primes[g_nprimes - 1];
}


static void		hash_table_resize	 (struct HashTable	*hash_table);
static struct HashNode**	hash_table_lookup_node (struct HashTable	*hash_table,const void *	 key);
static struct HashNode*	hash_node_new		 (void *key, void *value);
static void		hash_node_destroy	 (  struct HashNode	*hash_node);
static void		hash_nodes_destroy	 (  struct HashNode	*hash_node);




struct HashTable* hash_table_new (HashFunc        hash_func,CompareFunc key_compare_func)
{
	struct HashTable *hash_table;
	hash_table = (struct HashTable *)calloc (sizeof(struct HashTable), 1);
	hash_table->size = HASH_TABLE_MIN_SIZE;
	hash_table->nnodes = 0;
	hash_table->frozen = 0;
	hash_table->hash_func = hash_func;
	hash_table->key_compare_func = key_compare_func;
	hash_table->nodes = (struct HashNode **)calloc (sizeof(struct HashNode*), hash_table->size);
	return hash_table;
}


void hash_table_destroy (struct HashTable *hash_table)
{
  unsigned int i;
  
  if(!hash_table)
  	return;
  for (i = 0; i < hash_table->size; i++)
    hash_nodes_destroy (hash_table->nodes[i]);

  free(hash_table->nodes);
  free (hash_table);
}


static inline struct HashNode** hash_table_lookup_node (struct HashTable	*hash_table, const void *	 key)
{
	struct HashNode **node;
	node = &hash_table->nodes[(* hash_table->hash_func) (key) % hash_table->size];
	
	if (hash_table->key_compare_func)
	  while (*node && !(*hash_table->key_compare_func) ((*node)->key, key))
		node = &(*node)->next;
	else
	  while (*node && (*node)->key != key)
		node = &(*node)->next;
	return node;

}

void *hash_table_lookup (struct HashTable	 *hash_table,	  const void * key)
{
	struct HashNode *node;
	if(!hash_table || !key)
		return NULL;
	node = *hash_table_lookup_node (hash_table, key);
	return node ? node->value : NULL;

}



void hash_table_insert (struct HashTable *hash_table,void *key,void *value)
{
	struct HashNode **node;
	if(!hash_table)
  		return;
	node = hash_table_lookup_node (hash_table, key);
  	if (*node)
	{
		(*node)->value = value;
    }
	else
	{
		*node = hash_node_new (key, value);
		hash_table->nnodes++;
		if (!hash_table->frozen)
			hash_table_resize (hash_table);
	}
}



void hash_table_remove (struct HashTable	     *hash_table,const void *    key)
{
	struct HashNode **node, *dest;
	if(!hash_table)
		return;
	node = hash_table_lookup_node (hash_table, key);

	if (*node)
	{
		dest = *node;
		(*node) = dest->next;
		hash_node_destroy (dest);
		hash_table->nnodes--;
		if (!hash_table->frozen)
			hash_table_resize (hash_table);
    }
}




int hash_table_lookup_extended (struct HashTable	*hash_table,const void *	 lookup_key,void * *orig_key,void *		*value)
{
	struct HashNode *node;
	if(!hash_table)
		return 0;
	node = *hash_table_lookup_node (hash_table, lookup_key);
	
	if (node)
	{
		if (orig_key)
			*orig_key = node->key;
		if (value)
			*value = node->value;
		return 1;
	}
	else
	return 0;
}


void hash_table_freeze (struct HashTable *hash_table)
{
	if(!hash_table)
		return;
	hash_table->frozen++;
}



void hash_table_thaw (struct HashTable *hash_table)
{
	if(!hash_table)
		return;
	if (hash_table->frozen)
		if (!(--hash_table->frozen))
			hash_table_resize (hash_table);
}



int hash_table_foreach_remove (struct HashTable	*hash_table,HRFunc	 func,void *	 user_data)
{
	struct HashNode *node, *prev;
	unsigned int i;
	int deleted = 0;
  
	if(!hash_table)
		return 0;

  	for (i = 0; i < hash_table->size; i++)
	{
		restart:
		prev = NULL;
		for (node = hash_table->nodes[i]; node; prev = node, node = node->next)
		{
			if ((* func) (node->key, node->value, user_data))
			{
				deleted += 1;
				hash_table->nnodes -= 1;
				if (prev)
				{
					prev->next = node->next;
					hash_node_destroy (node);
					node = prev;
				}
				else
				{
					hash_table->nodes[i] = node->next;
					hash_node_destroy (node);
					goto restart;
				}
			}
		}
	}
	if (!hash_table->frozen)
		hash_table_resize (hash_table);
	return deleted;
}

void hash_table_foreach (struct HashTable *hash_table,HFunc	func,void *user_data)
{
	struct HashNode *node;
	int i;
	if(!hash_table || !func)
		return;
	for (i = 0; i < hash_table->size; i++)
		for (node = hash_table->nodes[i]; node; node = node->next)
		{
			(* func) (node->key, node->value, user_data);
		}
}



static void hash_table_resize (struct HashTable *hash_table)
{
	struct HashNode **new_nodes;
	struct HashNode *node;
	struct HashNode *next;
	float nodes_per_list;
	unsigned int hash_val;
	int new_size;
	int i;
	nodes_per_list = (float) hash_table->nnodes / (float) hash_table->size;
	if ((nodes_per_list > 0.3 || hash_table->size <= HASH_TABLE_MIN_SIZE) &&
		(nodes_per_list < 3.0 || hash_table->size >= HASH_TABLE_MAX_SIZE))
		return;
	new_size = CLAMP(g_spaced_primes_closest (hash_table->nnodes),
		HASH_TABLE_MIN_SIZE,
		HASH_TABLE_MAX_SIZE);

	new_nodes = (struct HashNode **)calloc (sizeof(struct HashNode), new_size);
	for (i = 0; i < hash_table->size; i++)
		for (node = hash_table->nodes[i]; node; node = next)
		{
			next = node->next;
			hash_val = (* hash_table->hash_func) (node->key) % new_size;
			node->next = new_nodes[hash_val];
			new_nodes[hash_val] = node;
		}
	free (hash_table->nodes);
	hash_table->nodes = new_nodes;
	hash_table->size = new_size;
}

static struct HashNode* hash_node_new (void * key,void * value)
{

	struct HashNode *hash_node = malloc(sizeof(struct HashNode));
	memset(hash_node,0,sizeof(struct HashNode));
	hash_node->key = key;
	hash_node->value = value;
	hash_node->next = NULL;
	return hash_node;
}



static void hash_node_destroy (struct HashNode *hash_node)
{
	if(hash_node){
		free(hash_node);
	}
}


static void hash_nodes_destroy (struct HashNode *hash_node)
{
	if (hash_node)
	{
		struct HashNode *node,*node0;
		node = hash_node;
		while (node)
		{
			node0 = node;
			node = node->next;
			free(node0);
		}
    }
}

