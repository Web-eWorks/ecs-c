#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ecs.h"
#include "profile.h"
#include "testcomponent.h"
#include "testsystem.h"

COMPONENT_IMPL(TestComponent, ComponentStorageNormal)
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
void TestSystem_update(Entity e, Component **c, TestSystem *system)
{
	// This will always assert true, but we check it here anyways.
	assert(c[0]);

	// Since we returned "TestComponent" as element 0 in TestSystem_collection,
	// c[0] will always be a TestComponent pointer, and thus it is
	// safe to cast like this.
	TestComponent *comp = c[0];
	assert(TestComponent_GetString(comp));
}
bool TestSystem_event(Event *event, TestSystem *system)
{
	return false;
}

EntityArchetype *TestEntityArchetype;
const char *TestEntity_components[] = {
	"TestComponent", NULL
};

const SystemUpdateInfo TestSystem_update_info = {
	true, false, false, NULL
};

#ifndef TEST_ENTITIES
#define TEST_ENTITIES 1000000
#endif

#ifndef TEST_REPS
#define TEST_REPS 60
#endif

int main (int argc, const char **argv)
{
	ECS *ecs = ECS_New();

	PERF_START();
	bool res = REGISTER_COMPONENT(ecs, TestComponent);
	assert(res);

	TestSystem_reg.archetype = ECS_EntityRegisterArchetype(ecs, "TestEntityArchetype", TestEntity_components);

	TestSystem *test_sys = malloc(sizeof(TestSystem));
	res = REGISTER_SYSTEM(ecs, TestSystem, test_sys);
	assert(res);

	res = ECS_SetThreads(ecs, 2);
	assert(res);

	PERF_PRINT_US("Initialization");
	printf("> Initialization done (1/4).\n");

	PERF_UPDATE();
	Entity entity;
	TestComponent *comp;
	hash_t comp_type = COMPONENT_ID(TestComponent);
	for (int i = 0; i < TEST_ENTITIES; i++) {
		entity = ECS_EntityNew(ecs, NULL);
		comp = ECS_EntityAddComponent(ecs, entity, comp_type);
		assert(comp);
	}

	PERF_PRINT_MS("Entity Creation");
	printf("> Setup done (2/4).\n");

	PERF_UPDATE();
	for (int i = 0; i < TEST_REPS; i++) {
		ECS_Update(ecs);
	}
	PERF_PRINT_MS("Updates");

	printf("> Update done (3/4).\n");

	PERF_UPDATE();
	ECS_Delete(ecs);
	free(test_sys);
	PERF_PRINT_MS("Shutdown");

	printf("> Shutdown done (4/4).\n");

	return 0;
}
