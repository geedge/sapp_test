

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <pcap/pcap.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include <MESA/stream.h>
#include <MESA/dns.h>
#include <MESA/cJSON.h>
#include <MESA/MESA_handle_logger.h>
#include <MESA/MESA_prof_load.h>


#include "dns_hash_table.h"
#include "dns_heap_sort.h"
#include "dns_hash_array.h"

#define CONPATH "./plug/business/dns_topn/dns_topn.conf"
#define DNS_TOPN_PLUGNAME 	"dns_topn.so"
#define LOG_PATH_DEFAULT  "./plug/business/dns_topn/dns_topn_log"

#define LOG_LEVEL_DEFAULT 10

#define MIN(a,b) ((a)<(b)?(a):(b))


struct InfoConfigSystem
{
	int topnum;
	void *log_handler;
	char  log_path[PATH_MAX];

	int thread_count;
};

struct HeapSortArrayNode
{
	char dnsname[256];
	int value;
};

struct KeyValue{
	void *key;
	void *value;
};


static struct InfoConfigSystem info_config_sys_dnstopn;

static struct HashTableArray * hash_table_array_dnstopn = NULL;



int hash_operation(const void *key)
{
	char *str = (char *)key;
	unsigned int hash = 0;
	int i;
	
	for (i=0; *str; i++)
	{
		if ((i & 1) == 0)
		{
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		}
		else
		{
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}
	
	return (hash & 0x7FFFFFFF);
}


int hash_key_compare (const void *a,const void *b)
{
	int ret = 0;
	ret = strcmp((char *)a, (char *)b);
	return !ret;
}


int hash_do_insert_node(struct HashTable *hash_table, void * key,void * value)
{
	char *key_hash_table;
	int  *value_hash_table;
	
	if(!hash_table || !key)
		return 1;
	
	void * value_found_hash_table = hash_table_lookup(hash_table,key);
	
	if(value_found_hash_table)
	{
		*((int *)value_found_hash_table) += *((int *)value); 
		return 0;
	}
	key_hash_table  = (char *)malloc((DNS_MAX_NAME + 1) * sizeof(char));
	value_hash_table = (int *)malloc(sizeof(int));
	
	memset(key_hash_table, 0, (DNS_MAX_NAME + 1) * sizeof(char));
	strcpy(key_hash_table,(char *)key);
	*value_hash_table = 1;	
	
	hash_table_insert (hash_table, (void *)key_hash_table, (void *)value_hash_table);
	return 0;
}



void hash_do_print_callback (void * key,void * value,void *user_data)
{
	printf("value=%d,key=%s\n",*((int *)value), (char *)key);
}


void hash_do_merge_callback (void * key,void * value,void *user_data)
{
	struct HashTable *hash_table;

	hash_table = (struct HashTable *)user_data;
	
	hash_do_insert_node(hash_table, key,value);
}


void hash_free_node_key_value_callback (void * key,void * value,void *user_data)
{
	if(key)
		free(key);
	
	if(value)
		free(value);
}



void hash_write_heapsort_arr_callback (void * key,void * value,void *user_data)
{	
	if(!key || !value || !user_data)
		return;

	struct KeyValue kv = {.key = key, .value = value};
	struct HeapSortArray * heap_sort_array = (struct HeapSortArray *)user_data;	
	heapsort_array_insert_node(heap_sort_array,sizeof(struct HeapSortArrayNode), (void *)&kv);	
}



int heap_sort_init_node_callback(void *node, void *userdata)
{
	struct KeyValue *kv;
	struct HeapSortArrayNode * heap_sort_node;

	kv = (struct KeyValue *)userdata;
	heap_sort_node= (struct HeapSortArrayNode *)node;
	
	strcpy(heap_sort_node->dnsname,(char *)kv->key);
	heap_sort_node->value = *((int *)kv->value);
	return 0;
}

int heap_sort_cmp_callback(void *a,void *b, int type)
{
	int ret = 0;
	struct HeapSortArrayNode * heap_sort_node_a;
	struct HeapSortArrayNode * heap_sort_node_b;

	heap_sort_node_a = (struct HeapSortArrayNode *)a;
	heap_sort_node_b = (struct HeapSortArrayNode *)b;
	
	switch(type)
	{
		case HEAP_SORT_EQUAL_TAG:
			ret = (heap_sort_node_a->value == heap_sort_node_b->value);
			break;
		case HEAP_SORT_LESS_TAG:
			ret = (heap_sort_node_a->value < heap_sort_node_b->value);
			break;
		case HEAP_SORT_GREATER_TAG:
			ret = (heap_sort_node_a->value > heap_sort_node_b->value);
			break;
		default:
			ret = 0;
	}
	
	return ret;
}


void heap_sort_print_callback(void *data)
{
	struct HeapSortArrayNode *heap_array_node;
	
	if(!data)
		return;
	heap_array_node = (struct HeapSortArrayNode *)data;
	printf("value=%d, Dnsname=%s\n",heap_array_node->value,heap_array_node->dnsname);
}

struct HashTable * hash_table_array_merge_hash_to_one(struct HashTableArray *hash_table_array,int index_merged)
{
	int i;
	
	if(index_merged >= hash_table_array->size)
		return NULL;

	for(i = 0; i < hash_table_array->size; i ++)
	{
		if(i == index_merged)
			continue;
		
		if((hash_table_array->nodes[i]).hash_table)
		{
			hash_table_foreach ((hash_table_array->nodes[i]).hash_table,hash_do_merge_callback,(void *)(hash_table_array->nodes[index_merged]).hash_table);
		}
	}
	return (hash_table_array->nodes[index_merged]).hash_table;
}


static void do_sort_in_heap_sort_arr_dnstopn(struct HeapSortArray * heap_sort_array)
{
	if(!heap_sort_array)
		return;
	heapsort_to_do_sort(heap_sort_array,heap_sort_array->size);
}


static struct HeapSortArray * do_save_info_heapsort_from_hash_dnstopn(struct HashTableArray * src_hash_table_array)
{
	struct HashTable * final_hash_table;
	struct HeapSortArray * heap_sort_array;
	
	final_hash_table = hash_table_array_merge_hash_to_one(src_hash_table_array, 0);
	if(!final_hash_table)
		return NULL;

	heap_sort_array = heapsort_array_new(heap_sort_init_node_callback,heap_sort_cmp_callback, final_hash_table->nnodes);
	if(!heap_sort_array)
		return NULL;

	hash_table_foreach(final_hash_table, hash_write_heapsort_arr_callback, heap_sort_array);

	return heap_sort_array;
}


static int do_save_info_to_hash_dnstopn(dns_info_t *dns_info,struct HashTable *hash_table)
{
	int value = 1;
	
	if(!dns_info->hdr_info.qr)
		return 0;
	MESA_handle_runtime_log(info_config_sys_dnstopn.log_handler,RLOG_LV_INFO, DNS_TOPN_PLUGNAME, "Info:handle package:dnsname=%s", (char *)dns_info->query_question.qname);

	hash_do_insert_node(hash_table,(void *)dns_info->query_question.qname, (void *)&value);


	return 0;
}


void do_print_heapsort_dnstopn(struct HeapSortArray *heap_sort_array, int topnum)
{
	heapsort_print(heap_sort_array, MIN(heap_sort_array->size,topnum), heap_sort_print_callback);
}


void do_free_all_dnstopn(struct HashTableArray * hash_table_array, struct HeapSortArray * heap_sort_array)
{
	if(hash_table_array)
		hash_table_array_destory(hash_table_array,hash_free_node_key_value_callback);
	
	if(heap_sort_array)
		heapsort_destory(heap_sort_array);
	
}


int read_config_and_init_dnstopn(struct InfoConfigSystem * info_conf_sys)
{
	memset(info_conf_sys, 0 ,sizeof(struct InfoConfigSystem));
	
	info_conf_sys->thread_count = get_thread_count();
	if (info_conf_sys->thread_count < 1)
	{
		MESA_handle_runtime_log(info_config_sys_dnstopn.log_handler, RLOG_LV_FATAL, DNS_TOPN_PLUGNAME, "Error:get thread count error");
		return -1;
	}
		
	MESA_load_profile_int_def(CONPATH, "DNS", "TOPNUM", &(info_conf_sys->topnum), 100);
	
	MESA_load_profile_string_def(CONPATH, "DNS", "TOPNUM_LOG_PATH", info_conf_sys->log_path, PATH_MAX, LOG_PATH_DEFAULT);
	
	info_conf_sys->log_handler = MESA_create_runtime_log_handle(info_conf_sys->log_path, RLOG_LV_DEBUG);

	if(info_conf_sys->log_handler == NULL)
	{
		printf("Error:<plugin:dns_topn> <info:get log handler error>\n");
		return -1;
	}
	return 0;
}



char dns_topn_entry (stSessionInfo *session_info,  void **pme, int thread_seq, struct streaminfo *a_udp, void *a_packet)
{
	dns_info_t *dns_info;
	struct HashTable *hash_table;
	
	dns_info   = (dns_info_t *)session_info->app_info;
	hash_table = hash_table_array_dnstopn->nodes[thread_seq].hash_table;

	if(dns_info!= NULL)
	{
		do_save_info_to_hash_dnstopn(dns_info, hash_table);
	}

	return PROT_STATE_GIVEME;
}


int dns_topn_init()
{	
	if(read_config_and_init_dnstopn(&info_config_sys_dnstopn) == -1)
		return -1;

	hash_table_array_dnstopn = hash_table_array_new(info_config_sys_dnstopn.thread_count);
	if(!hash_table_array_dnstopn)
	{
		hash_table_array_destory(hash_table_array_dnstopn, NULL);
		MESA_handle_runtime_log(info_config_sys_dnstopn.log_handler,RLOG_LV_FATAL, DNS_TOPN_PLUGNAME, "Fail to build hash table array");
		return -1;
	}
	
	hash_table_array_init(hash_table_array_dnstopn, hash_operation, hash_key_compare);
	return 0;		
}

void dns_topn_destory()
{
	struct HeapSortArray *heap_sort_array;

	heap_sort_array = do_save_info_heapsort_from_hash_dnstopn(hash_table_array_dnstopn);
	if(!heap_sort_array)
		return;
	
	do_sort_in_heap_sort_arr_dnstopn(heap_sort_array);
	
	do_print_heapsort_dnstopn(heap_sort_array,info_config_sys_dnstopn.topnum);
	
	do_free_all_dnstopn(hash_table_array_dnstopn,heap_sort_array);
	MESA_destroy_runtime_log_handle(info_config_sys_dnstopn.log_handler);
}

