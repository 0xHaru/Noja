#include "../utils/defs.h"
#include "objects.h"

typedef struct ClosureObject ClosureObject;

struct ClosureObject {
	Object         base;
	ClosureObject *prev;
	Object        *vars;
};

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp);
static Object *select(Object *self, Object *key, Heap *heap, Error *err);

static TypeObject t_closure = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "closure",
	.size = sizeof(ClosureObject),
	.select = select,
	.walk = walk,
};

Object *Object_NewClosure(Object *parent, Object *new_map, Heap *heap, Error *error)
{
	ClosureObject *obj = (ClosureObject*) Heap_Malloc(heap, &t_closure, error);

	if(obj == NULL)
		return NULL;

	if(parent != NULL && parent->type != &t_closure)
		{
			Error_Report(error, 0, "Object is not a closure");
			return NULL;
		}

	obj->prev = (ClosureObject*) parent;
	obj->vars = new_map;

	return (Object*) obj;
}

static Object *select(Object *self, Object *key, Heap *heap, Error *err)
{
	ClosureObject *closure = (ClosureObject*) self;

	Object *selected = NULL;

	while(closure != NULL && selected == NULL)
		{
			selected = Object_Select(closure->vars, key, heap, err);

			if(err->occurred)
				return NULL;

			closure = closure->prev;
		}

	return selected;
}

static void walk(Object *self, void (*callback)(Object **referer, void *userp), void *userp)
{
	ClosureObject *closure = (ClosureObject*) self;

	callback((Object**) &closure->prev, userp);
	callback(&closure->vars, userp);
}