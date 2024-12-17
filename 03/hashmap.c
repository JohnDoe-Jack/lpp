#include "lpp.h"

HashMap * newHashMap(int size)
{
  HashMap * hashmap;
  if ((hashmap = malloc(sizeof(HashMap))) == NULL) {
    error("Memory allocation error\n");
    exit(1);
  }
  hashmap->size = size;

  hashmap->entries = malloc(sizeof(Entry *) * size);
  if (hashmap->entries == NULL) {
    error("Memory allocation error\n");
    exit(1);
  }
  for (int i = 0; i < size; i++) {
    hashmap->entries[i] = NULL;
  }

  return hashmap;
}

static unsigned int hash(const char * key, const int size)
{
  unsigned int hash = 0;
  for (int i = 0; key[i] != '\0'; i++) {
    hash = 31 * hash + key[i];
  }
  return hash % size;
}

void insertToHashMap(const HashMap * hashmap, const char * key, ID * value)
{
  unsigned int index = hash(key, hashmap->size);

  Entry * entry = hashmap->entries[index];
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      entry->value = value;
      return;
    }
    entry = entry->next;
  }

  entry = (Entry *)malloc(sizeof(Entry));
  if (entry == NULL) {
    // メモリ確保に失敗した場合の処理
    fprintf(stderr, "Failed to allocate memory for new entry\n");
    exit(1);
  }

  entry->key = strdup(key);
  entry->value = value;
  entry->next = hashmap->entries[index];
  hashmap->entries[index] = entry;
}

ID * getValueFromHashMap(const HashMap * hashmap, const char * key)
{
  unsigned int index = hash(key, hashmap->size);

  Entry * entry = hashmap->entries[index];
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }

  return NULL;
}

static void freeEntry(Entry * entry)
{
  while (entry != NULL) {
    Entry * next = entry->next;
    free(entry->key);
    free(entry->value);
    free(entry);
    entry = next;
  }
}

void freeHashMap(HashMap * hashmap)
{
  for (int i = 0; i < hashmap->size; i++) {
    freeEntry(hashmap->entries[i]);
  }
  free(hashmap->entries);
  free(hashmap);
}

int removeFromHashMap(const HashMap * hashmap, const char * key)
{
  unsigned int index = hash(key, hashmap->size);
  Entry * entry = hashmap->entries[index];

  Entry * pred = NULL;
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      break;
    }
    pred = entry;
    entry = entry->next;
  }

  if (entry == NULL) return -1;
  if (pred == NULL) {
    hashmap->entries[index] = entry->next;
  } else {
    pred->next = entry->next;
  }

  free(entry->key);
  free(entry->value);
  free(entry);

  return 0;
}