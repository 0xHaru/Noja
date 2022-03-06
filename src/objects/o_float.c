#include <assert.h>
#include "objects.h"

static double to_float(Object *obj, Error *err);
static void print(Object *obj, FILE *fp);
static _Bool op_eql(Object *self, Object *other);

typedef struct {
	Object base;
	double val;
} FloatObject;

static TypeObject t_float = {
	.base = (Object) { .type = &t_type, .flags = Object_STATIC },
	.name = "float",
	.size = sizeof (FloatObject),
	.atomic = ATMTP_FLOAT,
	.to_float = to_float,
	.print = print,
	.op_eql = op_eql
};

static _Bool op_eql(Object *self, Object *other)
{
	assert(self != NULL);
	assert(self->type == &t_float);
	assert(other != NULL);
	assert(other->type == &t_float);

	FloatObject *i1, *i2;

	i1 = (FloatObject*) self;
	i2 = (FloatObject*) other;

	return i1->val == i2->val;
}

static double to_float(Object *obj, Error *err)
{
	assert(obj);
	assert(err);
	assert(Object_GetType(obj) == &t_float);

	(void) err;

	return ((FloatObject*) obj)->val;
}

Object *Object_FromFloat(double val, Heap *heap, Error *error)
{
	FloatObject *obj = (FloatObject*) Heap_Malloc(heap, &t_float, error);

	if(obj == 0)
		return 0;

	obj->val = val;

	return (Object*) obj;
}

static void print(Object *obj, FILE *fp)
{
	assert(fp != NULL);
	assert(obj != NULL);
	assert(obj->type == &t_float);

	fprintf(fp, "%2.2f", ((FloatObject*) obj)->val);
}