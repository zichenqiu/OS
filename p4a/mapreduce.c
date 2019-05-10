#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "mapreduce.h"

typedef struct __val_node_t {
	char *val;
	struct __val_node_t *next;
} val_node_t;

typedef struct __val_list_t {
	val_node_t *head;
	pthread_mutex_t lock;
	//iterator for get_next
	val_node_t *cur_val;
} val_list_t;

typedef struct __key_node_t {
	char *key;
	val_list_t *vals;
	struct __key_node_t *next;
} key_node_t;

typedef struct __key_list_t {
	key_node_t *head;
	pthread_mutex_t lock;
} key_list_t;

typedef struct __hash_t {
	key_list_t *partitions;
	unsigned long num_partitions;
} hash_t;

hash_t *map_table;
pthread_mutex_t fileLock;
int file_count;
char** files;
int num_files;
int num_partitions;
Mapper mapper;
Reducer reducer;
Partitioner partitioner;


static int map_copy(char **dest, char *source) {
	*dest = malloc(sizeof(char) * (strlen(source) + 1));
	if (*dest == NULL) {
		return -1;
	}
	strcpy(*dest, source);
	return 0;
}

static void val_list_init(val_list_t *list) {
	list->head = NULL;
	list->cur_val = NULL;
	pthread_mutex_init(&list->lock, NULL);
}

static void key_list_init(key_list_t *list) {
	list->head = NULL;
	pthread_mutex_init(&list->lock, NULL);
}


//Inserts a value in the specified val list
//This will release the lock on the key_list it came from while
//it iterates through the val_list (val_list has its own lock).
static int val_list_insert(val_list_t *val_list, key_list_t *key_list, char *val) {
	pthread_mutex_lock(&val_list->lock);
	pthread_mutex_unlock(&key_list->lock);
	
	//create new node
	val_node_t *new = malloc(sizeof(val_node_t));
	if (new == NULL) {
		perror("malloc: val_list_insert");
		pthread_mutex_unlock(&val_list->lock);
		return -1;
	}

	//insert at sorted position
	val_node_t *cur = val_list->head;
	if (cur == NULL || strcmp(val, cur->val) < 0) {
		//copy val, insert at head
		if (map_copy(&new->val, val) < 0) {
			perror("malloc: val_list_insert");
			free(new);
			pthread_mutex_unlock(&val_list->lock);
			return -1;
		}
		if (cur == NULL) {
			val_list->head = new;
			new->next = NULL;
		}
		else {
			new->next = val_list->head;
			val_list->head = new;
		}
	}
	else {
		//find place to insert, copy val if not already in list
		int cmp = strcmp(val, cur->val);
		while (cur->next != NULL && cmp > 0) {
			if (strcmp(val, cur->next->val) < 0) {
				break;
			}
			cur = cur->next;
			cmp = strcmp(val, cur->val);
		}
		if (cmp == 0) {
			new->val = cur->val;
		}
		else {
			if (map_copy(&new->val, val) < 0) {
				perror("malloc: val_list_insert");
				free(new);
				pthread_mutex_unlock(&val_list->lock);
				return -1;
			}
		}
		new->next = cur->next;
		cur->next = new;
	}

	//update iterator for get_next
	val_list->cur_val = val_list->head;

	pthread_mutex_unlock(&val_list->lock);
	return 0;
}

//Inserts a key/val pair into the specified key_list
//Will release the key_list lock when inserting the value into val_list
static int key_list_insert(key_list_t *key_list, char *key, char *val) {
	pthread_mutex_lock(&key_list->lock);

	key_node_t *cur = key_list->head;
	key_node_t *new;
	//if empty, create new node at head
	if (cur == NULL || strcmp(key, cur->key) < 0) {
		new = malloc(sizeof(key_node_t));
		if (new == NULL) {
			perror("malloc: key_list_insert");
			pthread_mutex_unlock(&key_list->lock);
			return -1;
		}
		if (map_copy(&new->key, key) < 0) {
			perror("malloc: key_list_insert");
			free(new);
			pthread_mutex_unlock(&key_list->lock);
			return -1;
		}
		if (cur == NULL) {
			key_list->head = new;
			new->next = NULL;
		}
		else {
			new->next = key_list->head;
			key_list->head = new;
		}
		new->vals = malloc(sizeof (val_list_t));
		val_list_init(new->vals);
		if (val_list_insert(new->vals, key_list, val) < 0) {
			return -1;
		}
	}
	//otherwise find existing node or sorted position to insert
	else {
		int cmp = strcmp(key, cur->key);

		while (cur->next != NULL && cmp > 0) {
			if (strcmp(key, cur->next->key) < 0) {
				break;
			}
			cur = cur->next;
			cmp = strcmp(key, cur->key);
		}
		if (cmp == 0) {
			//insert into existing val list
			if (val_list_insert(cur->vals, key_list, val) < 0) {
				return -1;
			}
		}
		else {
			new = malloc(sizeof(key_node_t));
			if (new == NULL) {
				perror("malloc: key_list_insert");
				pthread_mutex_unlock(&key_list->lock);
				return -1;
			}
			if (map_copy(&new->key, key) < 0) {
				perror("malloc: key_list_insert");
				free(new);
				pthread_mutex_unlock(&key_list->lock);
				return -1;
			}
			new->next = cur->next;
			cur->next = new;
			new->vals = malloc(sizeof (val_list_t));
			val_list_init(new->vals);
			if (val_list_insert(new->vals, key_list, val) < 0) {
				return -1;
			}
		}
	}
/*
	//DEBUG
	printf("after insertion, key_list: ");
	cur = key_list->head;
	while (cur != NULL) {
		printf(" %s ", cur->key);
		cur = cur->next;
	}
	printf("\n");
*/
	return 0;
}

int hash_init(hash_t *table, unsigned long num_partitions) {
	table->partitions = malloc(num_partitions * sizeof(key_list_t));
	if (table->partitions == NULL) {
		perror("malloc: hash_init");
		return -1;
	}
	table->num_partitions = num_partitions;
	for (unsigned long i = 0; i < num_partitions; ++i) {
		key_list_init(&table->partitions[i]);
	}
	return 0;
}

int hash_insert(hash_t *table, char *key, char *val)
{
	unsigned long partition = partitioner(key, table->num_partitions);
/*
	//DEBUG
	printf("hashing \"%s\" into partition #%lu\n", key, partition);
*/
	return key_list_insert(&table->partitions[partition], key, val);
}

void free_val_list(val_list_t *val_list) {
	//free each val_node and free the last of each unique val
	val_node_t *cur = val_list->head;
	val_node_t *next;

	while (cur != NULL) {
		next = cur->next;
		if (next == NULL || cur->val != next->val) {
			free(cur->val);
		}
		free(cur);
		cur = next;
	}
	free(val_list);
}

void *free_hash_partition(void *partition) {
	key_node_t *cur = ((key_list_t *) partition)->head;
	key_node_t *next;

	while (cur != NULL) {
		next = cur->next;
		free_val_list(cur->vals);
		free(cur->key);
		free(cur);
		cur = next;
	}
	return NULL;
}

void *map_routine(void *args){

	while(1){
		char *current_file;
		pthread_mutex_lock(&fileLock);
		if (file_count >= num_files) {
			// finished reading all files
			pthread_mutex_unlock(&fileLock);
			return NULL;
		} 
		current_file = files[file_count++];
		pthread_mutex_unlock(&fileLock);
		mapper(current_file);
   	}
	return NULL;
}

char *get_next(char *key, int partition_number) {
	//find key_node
	int cmp;
	val_list_t *cur_list;
	key_node_t *cur_key = map_table->partitions[partition_number].head;
	while (cur_key != NULL) {
		cmp = strcmp(key, cur_key->key);
		if (cmp == 0) {
			cur_list = cur_key->vals;
			if (cur_list->cur_val != NULL) {
				char *val = cur_list->cur_val->val;
				cur_list->cur_val = cur_list->cur_val->next;
				return val;
			}
			else { return NULL; }
		}
		else if (cmp < 0) {
			//past the point where key would be in sorted list
			break;
		}
		cur_key = cur_key->next;
	}
	//key not found in partition
	return NULL;
}

void *reduce_routine(void *partition) {
	key_node_t *cur = ((key_list_t *) partition)->head;
	unsigned long partition_number =  ((key_list_t *) partition) - map_table->partitions;
	while (cur != NULL) {
/*
		//DEBUG
		printf("about to reduce \"%s\" in partition #%lu\n", cur->key, partition_number);
*/
		reducer(cur->key, get_next, partition_number);
		cur = cur->next;
	}
	return NULL;
}


void MR_Emit(char *key, char *value) {
	//TODO: Implement emitter
	//printf("Enter MR_Emit");
	hash_insert(map_table, key, value);
	return;

/*	unsigned long partition_bucket;
	if (partitioner != NULL) {
		partition_bucket = partitioner(key, num_partitions);
	} else {
		partition_bucket = MR_DefaultHashPartition(key,num_partitions);
	}
	unsigned long pos = MR_DefaultHashPartition(key, 1024);
	pthread_mutex_lock(&table[partition_bucket].words[pos].lock);

	//create a node to hold the value
	node_v* new_node_v = malloc(sizeof(node_v));
	if (new_node_v == NULL) {
		perror("malloc: list_insert");
		pthread_mutex_unlock(&table[partition_bucket].words[pos].lock);
		return; 
	}

	new_node_v->value = malloc(sizeof(char)*216);
	if (new_node_v->value == NULL) {
		perror("malloc: value");
	}
	strcpy(new_node_v->value, value);
	new_node_v->next = NULL; 


	node_t* head_prev = table[partition_bucket].words[pos].head;
	while(head_prev != NULL) {
		if (strcmp(head_prev->key, key) == 0){	
			break;
		}
		head_prev = head_prev->next;
	}
	if (head_prev == NULL) {
		//create a new key-node
		node_t *new_node_k = malloc(sizeof(node_t));
		if(new_node_k == NULL) {
			perror("malloc: new_node_t");
			pthread_mutex_unlock(&table[partition_bucket].words[pos].lock);
			return;
		}
		new_node_k->head = new_node_v;
		new_node_k->next = table[partition_bucket].words[pos].head; //(TODO: sort key in reduce_thread)
		table[partition_bucket].words[pos].head = new_node_k;

		new_node_k->key = malloc(sizeof(char)*216);
		if(new_node_k->key == NULL) {
			perror("malloc: new_node_k");
		}
		strcpy(new_node_k->key, key);
	} else {
		//already existing node for the same key
		new_node_v->next = head_prev->head;
		head_prev->head = new_node_v; //add into the front (TODO: sort value in reduce_thread)
	}
	pthread_mutex_unlock(&table[partition_bucket].words[pos].lock); */

}


void MR_Run(int argc, char *argv[],
			Mapper map, int num_mappers,
			Reducer	reduce, int num_reducers,
			Partitioner partition) {
	
	//Initializaiton
	mapper = map;
	reducer = reduce;
	if (partition != NULL) {
		partitioner = partition;
	} else {
		partitioner = MR_DefaultHashPartition;
	}
	//
	file_count = 0; 
	files = (argv + 1);
	num_files = argc - 1;
	num_partitions = num_reducers;
	//
	map_table = malloc(sizeof(hash_t));
	hash_init(map_table, num_partitions);

	
/*
        for (int i = 0; i < num_reducers; i++) {
		pthread_mutex_init(&table[i].lock, NULL);
		table[i].sorted = NULL;
                for (int j = 0; j < 1024; j++) {
			table[i].words[j].head = NULL;
			pthread_mutex_init(&table[i].words[j].lock, NULL);
		}

	}
*/

	//pthread_t *mappers;
	pthread_t mappers[num_mappers]; 
 	for (int i = 0; i < num_mappers; i++) {
		pthread_create(&mappers[i], NULL, map_routine, (void*) files);
	}
	//wait for all the mappers to finish
	for (int i = 0; i < num_mappers; i++) {
		pthread_join(mappers[i], NULL);
	}	

	//pthread_t *reducers
	pthread_t reducers[num_reducers];
	for (int i = 0; i < num_reducers; i++) {
		pthread_create(&reducers[i], NULL, reduce_routine, (void *) &map_table->partitions[i]);
	}
	//wait for all the reducers to finish
	for (int i = 0; i < num_reducers; i++) {
		pthread_join(reducers[i], NULL);
	}

	//memory cleaning
	pthread_t cleaners[num_partitions];
	for (int i = 0; i < num_partitions; ++i) {
		pthread_create(&cleaners[i], NULL, free_hash_partition, (void *) &map_table->partitions[i]);
	}
	//wait for all cleaners to finish
	for (int i = 0; i < num_partitions; ++i) {
		pthread_join(cleaners[i], NULL);
	}
	//and then free map_table
	free(map_table->partitions);
	free(map_table);

	return;
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}
