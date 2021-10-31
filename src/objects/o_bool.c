#include <assert.h>
#include <string.h>
#include "objects.h"

static _Bool to_bool(Object *obj, Error *err);

static const Type t_int = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "int",
	.size = sizeof (Object),
	.atomic = ATMTP_BOOL,
	.to_bool = to_bool,
};

static Object the_true_object = {
	.type = t_bool,
	.flags = Object_STATIC,
}

static Object the_false_object = {
	.type = t_bool,
	.flags = Object_STATIC,
}

static _Bool to_bool(Object *obj, Error *err)
{
	assert(obj);
	assert(err);
	assert(Object_GetType(obj) == &t_bool);
	return obj == &the_true_object;
}

Object *Object_FromBool(_Bool val, Heap *heap, Error *error)
{
	(void) heap;
	(void) error;
	return val ? &the_true_object : &the_false_object;
}