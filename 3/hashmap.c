#include "lpp.h"

/**
 * @brief 新しいハッシュマップを作成する
 * 
 * @param size ハッシュマップのサイズ
 * @return HashMap* 作成されたハッシュマップへのポインタ
 */
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

/**
 * @brief ハッシュ値を計算する
 * 
 * @param key ハッシュ値を計算する文字列
 * @param size ハッシュマップのサイズ
 * @return unsigned int 
 */
static unsigned int hash(const char * key, const int size)
{
  unsigned int hash = 0;
  for (int i = 0; key[i] != '\0'; i++) {
    hash = 31 * hash + key[i];
  }
  return hash % size;
}

/**
 * @brief ハッシュマップにエントリを追加する
 * 
 * @param hashmap 挿入されるハッシュマップ
 * @param key キー
 * @param value キーに対応するバリュー
 */
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
    fprintf(stderr, "Failed to allocate memory for new entry\n");
    exit(1);
  }

  entry->key = strdup(key);
  entry->value = value;
  entry->next = hashmap->entries[index];
  hashmap->entries[index] = entry;
}

/**
 * @brief ハッシュマップから指定されたキーを持つエントリを取得する
 * 
 * @param hashmap 取得したいエントリが含まれるハッシュマップ
 * @param key 検索するキー
 * @return ID* 取得したエントリーの要素
 */
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

/**
 * @brief エントリーを解放する
 * 
 * @param entry 開放するエントリ
 */
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

/**
 * @brief ハッシュマップを解放する
 * 
 * @param hashmap 開放するハッシュマップ
 */
void freeHashMap(HashMap * hashmap)
{
  for (int i = 0; i < hashmap->size; i++) {
    freeEntry(hashmap->entries[i]);
  }
  free(hashmap->entries);
  free(hashmap);
}