#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ecs.h"
#include "testcomponent.h"
#include "testsystem.h"

COMPONENT_IMPL(TestComponent)
void TestComponent_new(TestComponent *comp) {
	comp->string = malloc(16);
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

SYSTEM_IMPL(TestSystem)
void TestSystem_update(Component **c, TestSystem *system)
{
	// Since we returned "TestComponent" as element 0 in TestSystem_collection,
	// c[0] will _always_ be a TestComponent pointer.
	TestComponent *comp = c[0];
	puts(TestComponent_GetString(comp));
}
bool TestSystem_event(Event *event, TestSystem *system)
{
	return false;
}
const char* TestSystem_collection[2] = {
	"TestComponent",
	NULL
};

int main (int argc, const char **argv)
{
	ECS *ecs = ECS_New();

	bool res = REGISTER_COMPONENT(ecs, TestComponent);
	assert(res);

	TestSystem *test_sys = malloc(sizeof(TestSystem));
	res = REGISTER_SYSTEM(ecs, TestSystem, test_sys);

	printf("Initialization done (1/4).\n");

	ComponentInfo *comp = ECS_ComponentNew(ecs, "TestComponent");
	assert(comp);
	Entity *entity = ECS_EntityNew(ecs);
	assert(entity);

	res = ECS_EntityAddComponent(entity, comp);
	assert(res);
	printf("Component addr: %p, Entity Component addr: %p.\n", comp,
		ECS_EntityGetComponentOfType(entity, hash_string("TestComponent"), 0));

	char *str = (char *)ECS_ComponentToString(ecs, comp);
	printf("Testing component ToString: %s\n", str); free(str);
	str = (char *)ECS_EntityToString(entity);
	printf("Testing entity ToString: %s\n", str); free(str);

	printf("Setup done (2/4).\n");

	ECS_UpdateBegin(ecs);

	ECS_UpdateSystems(ecs);

	ECS_UpdateEnd(ecs);

	printf("Update done (3/4).\n");

	ECS_EntityDelete(entity);

	ECS_SystemUnregister(ecs, "TestSystem");
	free(test_sys);

	ECS_Delete(ecs);

	printf("Shutdown done (4/4).\n");

	return 0;
}
