#include "SortedList.h"

#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>

#define MAX_LENGTH 10
#define MIN_LENGTH 3

//////////////////////////////////////// UTILITIES ////////////////////////////////////////    
// point two nodes at each other, making them consecutive in a list
inline void point_at(SortedListElement_t *first, SortedListElement_t *second);
// put the node "middle" between "first" and "second"
inline void put_between(SortedListElement_t *first, SortedListElement_t *second, 
			SortedListElement_t *middle);
// checks that the node is not corrupt
inline int is_valid_node(SortedListElement_t *node);
// checks if list is valid
inline int is_valid_list(SortedList_t *list);
//////////////////////////////////////// INTERFACE ////////////////////////////////////////    

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
  SortedList_t *ptr = list;
  if (! is_valid_list(list) || element == NULL || element->key == NULL) return; 
  do { 
  if (opt_yield & INSERT_YIELD) sched_yield();
    if (! is_valid_node(ptr->next)) return; 
    else ptr = ptr->next;
  } while (ptr != list && strcmp(ptr->key, element->key) < 0);
  put_between(ptr->prev, ptr, element);
}

int SortedList_delete(SortedListElement_t *element) {
  if (opt_yield & DELETE_YIELD) sched_yield();
  if (! is_valid_node(element)) return 1;
  else point_at(element->prev, element->next);
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  SortedList_t *ptr = list;
  if (! is_valid_list(ptr)) return NULL;
  do {
    if (opt_yield & LOOKUP_YIELD) sched_yield();
    if (! is_valid_node(ptr->next)) return NULL;
    else ptr = ptr->next;
  } while (ptr != list && strcmp(key, ptr->key) != 0);
  return ptr == list ? NULL : ptr;
}

int SortedList_length(SortedList_t *list) {
  SortedList_t *ptr = list->next;
  int length;
  if (! is_valid_node(ptr)) return -1; 
  for (length = 0; ptr != list; length++) {
    if (opt_yield & LOOKUP_YIELD) sched_yield();
    if (! is_valid_node(ptr->next)) return -1; 
    else ptr = ptr->next;
  }
  return length;
}


///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// UTILITIES ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void point_at(SortedListElement_t *first, SortedListElement_t *second) {
  first->next = second;
  second->prev = first;
}

void put_between(SortedListElement_t *first, SortedListElement_t *second, 
		       SortedListElement_t *middle) {
  point_at(first, middle);
  point_at(middle, second);
}

int is_valid_node(SortedListElement_t *node) {
  return (is_valid_list(node) && 
	  (node->key == NULL 
	   || (node->next != node && node->prev != node && 
	       strlen(node->key) <= MAX_LENGTH && strlen(node->key) >= MIN_LENGTH)));
}

int is_valid_list(SortedList_t *list) {
  return (list != NULL && list->next != NULL && list->prev != NULL && 
	  list->next->prev != NULL && list->prev->next != NULL &&
	  list->next->prev == list && list->prev->next == list);
}
