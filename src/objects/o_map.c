#include <assert.h>
#include "../utils/defs.h"
#include "objects.h"

typedef struct {
	Object base;
	int mapper_size, count;
	int *mapper;
	Object **keys;
	Object **vals;
} MapObject;

static Object *select(Object *self, Object *key, Heap *heap, Error *err);
static _Bool   insert(Object *self, Object *key, Object *val, Heap *heap, Error *err);
static int     count(Object *self);

static const Type t_map = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "map",
	.size = sizeof (MapObject),
	.select = select,
	.insert = insert,
	.count = count,
};

static inline int calc_capacity(int mapper_size)
{
	return mapper_size * 2.0 / 3.0;
}

Object *Object_NewMap(int num, Heap *heap, Error *error)
{
	// Handle default args.
	if(num < 0)
		num = 0;

	// Calculate initial mapper size.
	int mapper_size, capacity;
	{
		mapper_size = 8;
		while(calc_capacity(mapper_size) < num)
			mapper_size <<= 1;

		capacity = calc_capacity(mapper_size);
	}

	// Make the thing.
	MapObject *obj = (MapObject*) Heap_Malloc(heap, &t_map, error);
	{
		if(obj == 0)
			return 0;

		obj->mapper_size = mapper_size;
		obj->count = 0;
		obj->mapper = Heap_RawMalloc(heap, sizeof(int) * capacity, error);
		obj->keys   = Heap_RawMalloc(heap, sizeof(Object*) * capacity, error);
		obj->vals   = Heap_RawMalloc(heap, sizeof(Object*) * capacity, error);

		if(obj->mapper == NULL || obj->keys == NULL || obj->vals == NULL)
			return NULL;
	}

	for(int i = 0; i < mapper_size; i += 1)
		obj->mapper[i] = -1;

	return (Object*) obj;
}

static Object *select(Object *self, Object *key, Heap *heap, Error *error)
{
	assert(self != NULL);
	assert(self->type == &t_map);
	assert(key != NULL);
	assert(heap != NULL);
	assert(error != NULL);

	MapObject *map = (MapObject*) self;

	int mask = map->mapper_size - 1;
	int hash = Object_Hash(key, error);
	int pert = hash;

	if(error->occurred)
		// No hash function.
		return 0;

	int i = hash & mask;

	while(1)
		{
			int k = map->mapper[i];

			if(k == -1)
				{
					// Empty slot. 
					// This key is not present.
					return NULL;
				}
			else
				{
					// Found an item. 
					// Is it the right one?
					
					assert(k >= 0);

					if(Object_Compare(key, map->keys[i], error))
						// Found it!
						return map->vals[i];

					if(error->occurred)
						// Key doesn't implement compare.
						return 0;

					// Not the one we wanted.
				}

			pert >>= 5;
			i = (i * 5 + pert + 1) & mask;
		}

	UNREACHABLE;
	return NULL;
}

static _Bool grow(MapObject *map, Heap *heap, Error *error)
{
	assert(map != NULL);

	int new_mapper_size = map->mapper_size << 1;
	int new_capacity = calc_capacity(new_mapper_size);

	int *mapper   = Heap_RawMalloc(heap, sizeof(int) * new_mapper_size, error);
	Object **keys = Heap_RawMalloc(heap, sizeof(Object*) * new_capacity, error);
	Object **vals = Heap_RawMalloc(heap, sizeof(Object*) * new_capacity, error);

	if(mapper == NULL || keys == NULL || vals == NULL)
		return 0;

	for(int i = 0; i < map->count; i += 1)
		{
			keys[i] = map->keys[i];
			vals[i] = map->vals[i];
		}

	for(int i = 0; i < new_mapper_size; i += 1)
		mapper[i] = -1;

	// Rehash everything.
	for(int i = 0; i < map->count; i += 1)
		{
			// This won't trigger an error because the key
			// surely has a hash method since we already
			// hashed it once.
			int hash = Object_Hash(keys[i], error);
			assert(error->occurred == 0);

			int mask = new_mapper_size - 1;
			int pert = hash;
			
			int j = hash & mask;

			while(1)
				{
					if(mapper[j] == -1)
						{
							// No collision.
							// Insert here.
							mapper[j] = i;
							break;
						}

					// Collided. Find a new place.
					pert >>= 5;
					j = (j * 5 + pert + 1) & mask;
				}
		}

	// Done.
	map->mapper = mapper;
	map->mapper_size = new_mapper_size;
	map->keys = keys;
	map->vals = vals;
	return 1;
}

static _Bool insert(Object *self, Object *key, Object *val, Heap *heap, Error *error)
{
	assert(error != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(heap != NULL);
	assert(self != NULL);
	assert(self->type == &t_map);

	MapObject *map = (MapObject*) self;

	if(map->count == calc_capacity(map->mapper_size))
		if(!grow(map, heap, error))
			return 0;

	int mask = map->mapper_size - 1;
	int hash = Object_Hash(key, error);
	int pert = hash;

	if(error->occurred)
		// No hash function.
		return 0;

	int i = hash & mask;

	while(1)
		{
			int k = map->mapper[i];

			if(k == -1)
				{
					// Empty slot. We can insert it here.
					Object *key_copy = Object_Copy(key, heap, error);

					if(key_copy == NULL)
						return NULL;

					map->mapper[i] = map->count;
					map->keys[map->count] = key_copy;
					map->vals[map->count] = val;
					map->count += 1;
					return 1;
				}
			else
				{
					assert(k >= 0);

					if(Object_Compare(key, map->keys[i], error))
						{
							// Already inserted.
							// Overwrite the value.
							map->vals[i] = val;
							return 1;
						}

					if(error->occurred)
						// Key doesn't implement compare.
						return 0;

					// Collision.
				}

			pert >>= 5;
			i = (i * 5 + pert + 1) & mask;
		}

	UNREACHABLE;
	return 0;
}

static int count(Object *self)
{
	MapObject *map = (MapObject*) self;

	return map->count;
}