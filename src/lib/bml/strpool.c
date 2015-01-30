/*
 *    strmap version 2.0.1
 *
 *    ANSI C hash table for strings.
 *
 *	  Version history:
 *	  1.0.0 - initial release
 *	  2.0.0 - changed function prefix from strmap to sm to ensure
 *	      ANSI C compatibility 
 *	  2.0.1 - improved documentation 
 *
 *    strmap.c
 *
 *    Copyright (c) 2009, 2011, 2013 Per Ola Kristensson.
 *
 *    Per Ola Kristensson <pok21@cam.ac.uk> 
 *    Inference Group, Department of Physics
 *    University of Cambridge
 *    Cavendish Laboratory
 *    JJ Thomson Avenue
 *    CB3 0HE Cambridge
 *    United Kingdom
 *
 *    strmap is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    strmap is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with strmap.  If not, see <http://www.gnu.org/licenses/>.
 */  
#include "strpool.h"
typedef struct Pair Pair;
 typedef struct Bucket Bucket;
 struct Pair
{
  char *key;
   char *value;
 };
 struct Bucket
{
  unsigned int count;
   Pair * pairs;
 };
 struct StrPool
{
  unsigned int count;
   Bucket * buckets;
 };
 static Pair *get_pair (Bucket * bucket, const char *key);
static unsigned long hash (const char *str);
 StrPool * sp_new (unsigned int capacity) 
{
  StrPool * pool;
   pool = malloc (sizeof (StrPool));
  if (pool == NULL) {
    return NULL;
  }
  pool->count = capacity;
  pool->buckets = malloc (pool->count * sizeof (Bucket));
  if (pool->buckets == NULL) {
    free (pool);
    return NULL;
  }
  memset (pool->buckets, 0, pool->count * sizeof (Bucket));
  return pool;
}

 void
sp_delete (StrPool * pool) 
{
  unsigned int i, j, n, m;
  Bucket * bucket;
  Pair * pair;
   if (pool == NULL) {
    return;
  }
  n = pool->count;
  bucket = pool->buckets;
  i = 0;
  while (i < n) {
    m = bucket->count;
    pair = bucket->pairs;
    j = 0;
    while (j < m) {
      free (pair->key);
      free (pair->value);
      pair++;
      j++;
    }
    free (bucket->pairs);
    bucket++;
    i++;
  }
  free (pool->buckets);
  free (pool);
}

 int
sp_exists (const StrPool * pool, const char *key) 
{
  unsigned int index;
  Bucket * bucket;
  Pair * pair;
   if (pool == NULL) {
    return 0;
  }
  if (key == NULL) {
    return 0;
  }
  index = hash (key) % pool->count;
  bucket = &(pool->buckets[index]);
  pair = get_pair (bucket, key);
  if (pair == NULL) {
    return 0;
  }
  return 1;
}

 static const char *
sp_get (const StrPool * pool, const char *key) 
{
  unsigned int index;
  Bucket * bucket;
  Pair * pair;
   if (pool == NULL) {
    return NULL;
  }
  if (key == NULL) {
    return NULL;
  }
  index = hash (key) % pool->count;
  bucket = &(pool->buckets[index]);
  pair = get_pair (bucket, key);
  if (pair == NULL) {
    return NULL;
  }
  return pair->value;
}

 static const char *
sp_put (StrPool * pool, const char *key, const char *value) 
{
  unsigned int key_len, value_len, index;
  Bucket * bucket;
  Pair * tmp_pairs, *pair;
  char *tmp_value;
  char *new_key, *new_value;
   if (pool == NULL) {
    return NULL;
  }
  if (key == NULL || value == NULL) {
    return NULL;
  }
  key_len = strlen (key);
  value_len = strlen (value);
  
      /* Get a pointer to the bucket the key string hashes to */ 
      index = hash (key) % pool->count;
  bucket = &(pool->buckets[index]);
  
      /* Check if we can handle insertion by simply replacing
       * an existing value in a key-value pair in the bucket.
       */ 
      if ((pair = get_pair (bucket, key)) != NULL) {
    
        /* The bucket contains a pair that matches the provided key,
         * change the value for that pair to the new value.
         */ 
        if (strlen (pair->value) < value_len) {
      
          /* If the new value is larger than the old value, re-allocate
           * space for the new larger value.
           */ 
          tmp_value = realloc (pair->value, (value_len + 1) * sizeof (char));
      if (tmp_value == NULL) {
        return NULL;
      }
      pair->value = tmp_value;
    }
    
        /* Copy the new value into the pair that matches the key */ 
        strcpy (pair->value, value);
    return pair->value;
  }
  
      /* Allocate space for a new key and value */ 
      new_key = malloc ((key_len + 1) * sizeof (char));
  if (new_key == NULL) {
    return NULL;
  }
  new_value = malloc ((value_len + 1) * sizeof (char));
  if (new_value == NULL) {
    free (new_key);
    return NULL;
  }
  
      /* Create a key-value pair */ 
      if (bucket->count == 0) {
    
        /* The bucket is empty, lazily allocate space for a single
         * key-value pair.
         */ 
        bucket->pairs = malloc (sizeof (Pair));
    if (bucket->pairs == NULL) {
      free (new_key);
      free (new_value);
      return NULL;
    }
    bucket->count = 1;
  }
  
  else {
    
        /* The bucket wasn't empty but no pair existed that matches the provided
         * key, so create a new key-value pair.
         */ 
        tmp_pairs =
        realloc (bucket->pairs, (bucket->count + 1) * sizeof (Pair));
    if (tmp_pairs == NULL) {
      free (new_key);
      free (new_value);
      return NULL;
    }
    bucket->pairs = tmp_pairs;
    bucket->count++;
  }
  
      /* Get the last pair in the chain for the bucket */ 
      pair = &(bucket->pairs[bucket->count - 1]);
  pair->key = new_key;
  pair->value = new_value;
  
      /* Copy the key and its value into the key-value pair */ 
      strcpy (pair->key, key);
  strcpy (pair->value, value);
  return pair->value;
}

 const char *
sp_intern (StrPool * pool, const char *key) 
{
  const char *res;
   if (!(res = sp_get (pool, key))) {
    res = sp_put (pool, key, key);
  }
  return res;
}

 int
sp_get_count (const StrPool * pool) 
{
  unsigned int i, j, n, m;
  unsigned int count;
  Bucket * bucket;
  Pair * pair;
   if (pool == NULL) {
    return 0;
  }
  bucket = pool->buckets;
  n = pool->count;
  i = 0;
  count = 0;
  while (i < n) {
    pair = bucket->pairs;
    m = bucket->count;
    j = 0;
    while (j < m) {
      count++;
      pair++;
      j++;
    }
    bucket++;
    i++;
  }
  return count;
}

 int
sp_enum (const StrPool * pool, sp_enum_func enum_func, const void *obj) 
{
  unsigned int i, j, n, m;
  Bucket * bucket;
  Pair * pair;
   if (pool == NULL) {
    return 0;
  }
  if (enum_func == NULL) {
    return 0;
  }
  bucket = pool->buckets;
  n = pool->count;
  i = 0;
  while (i < n) {
    pair = bucket->pairs;
    m = bucket->count;
    j = 0;
    while (j < m) {
      enum_func (pair->key, pair->value, obj);
      pair++;
      j++;
    }
    bucket++;
    i++;
  }
  return 1;
}

 
/*
 * Returns a pair from the bucket that matches the provided key,
 * or null if no such pair exist.
 */ 
static Pair *
get_pair (Bucket * bucket, const char *key) 
{
  unsigned int i, n;
  Pair * pair;
   n = bucket->count;
  if (n == 0) {
    return NULL;
  }
  pair = bucket->pairs;
  i = 0;
  while (i < n) {
    if (pair->key != NULL && pair->value != NULL) {
      if (strcmp (pair->key, key) == 0) {
        return pair;
      }
    }
    pair++;
    i++;
  }
  return NULL;
}

 
/*
 * Returns a hash code for the provided string.
 */ 
    static unsigned long
hash (const char *str) 
{
  unsigned long hash = 5381;
  int c;
   while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

 
