#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ecs.h"
#include "profile.h"
#include "testcomponent.h"
#include "testsystem.h"

COMPONENT_IMPL(TestComponent)
void TestComponent_new(TestComponent *comp)
{
	comp->string = malloc(16);
	if (comp->string) strcpy(comp->string, "This is a test.");
}
void TestComponent_free(TestComponent *comp)
{
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
	// c[0] will _always_ be a TestComponent pointer, and thus it is safe to
	// cast like this.
	TestComponent *comp = c[0];
	assert(comp && comp->string);
}
bool TestSystem_event(Event *event, TestSystem *system)
{
	return false;
}

const char* TestSystem_collection[2] = {
	"TestComponent",
	NULL
};

#ifndef TEST_REPS
#define TEST_REPS 64000
#endif

int main (int argc, const char **argv)
{
	ECS *ecs = ECS_New();

	PERF_START();
	bool res = REGISTER_COMPONENT(ecs, TestComponent);
	assert(res);

	TestSystem *test_sys = malloc(sizeof(TestSystem));
	res = REGISTER_SYSTEM(ecs, TestSystem, test_sys);
	assert(res);

	PERF_PRINT_US("Initialization");
	printf("> Initialization done (1/4).\n");

	PERF_UPDATE();
	ComponentInfo *comp;
	Entity *entity;
	for (int i = 0; i < TEST_REPS; i++) {
		comp = ECS_ComponentNew(ecs, "TestComponent");
		assert(comp);
		entity = ECS_EntityNew(ecs);
		assert(entity);

		res = ECS_EntityAddComponent(entity, comp);
		assert(res);
	}

	PERF_PRINT_MS("Entity Creation");
	printf("> Setup done (2/4).\n");

	PERF_UPDATE();
	ECS_UpdateBegin(ecs);

	ECS_UpdateSystems(ecs);

	ECS_UpdateEnd(ecs);
	PERF_PRINT_MS("One cycle");

	printf("> Update done (3/4).\n");

	PERF_UPDATE();
	ECS_Delete(ecs);
	free(test_sys);
	PERF_PRINT_MS("Shutdown");

	printf("> Shutdown done (4/4).\n");

	return 0;
}
