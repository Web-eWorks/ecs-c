#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ecs.h"
#include "testcomponent.h"
#include "testsystem.h"

COMPONENT_IMPL(TestComponent)
void TestComponent_new(TestComponent *comp) {
	comp->string = malloc(15);
	if (comp->string) strcpy(comp->string, "This is a test.");
}
void TestComponent_free(TestComponent *comp) {
	if (comp->string) free(comp->string);
}
const char* TestComponent_GetString(TestComponent *comp)
{
	if (!comp || !comp->string) return NULL;
	return comp->string;
}

const char* collection[1] = {
	"TestComponent"
};

SYSTEM_IMPL(TestSystem)
void TestSystem_update(Component **c, TestSystem *system)
{
	// Since we returned "TestComponent" as element 0 in TestSystem_collection,
	// c[0] will _always_ be a TestComponent pointer.
	TestComponent *comp = c[0];
	puts(TestComponent_GetString(comp));
}
const char** TestSystem_collection(int *size, TestSystem *system)
{
	*size = 1;
	return collection;
}
bool TestSystem_event(Event *event, TestSystem *system)
{
	return false;
}

int main (int argc, const char **argv) {

	const char *str = "This is a test.";
	const hash_t str_hash = hash_string(str);

	printf("%s -> %x\n", str, str_hash);

	ECS *ecs = ECS_New();

	bool res = REGISTER_COMPONENT(ecs, TestComponent);
	assert(res);
	ComponentInfo *comp = ECS_ComponentNew(ecs, "TestComponent");
	assert(comp);

	printf("Part 1/3 done.\n");

	const char *comp_str = ECS_ComponentToString(ecs, comp);
	puts(comp_str);
	free((char *)comp_str);

	Entity *entity = ECS_EntityNew(ecs);
	assert(entity);
	res = ECS_EntityAddComponent(entity, comp);
	assert(res);

	const char *ent_str = ECS_EntityToString(entity);
	puts(ent_str);
	free((char *)ent_str);

	printf("Part 2/3 done.\n");

	ECS_EntityDelete(entity);

	ECS_Delete(ecs);

	printf("Part 3/3 done.\n");

	return 0;
}
