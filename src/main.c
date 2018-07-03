#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ecs.h"

COMPONENT(TestComponent)
struct TestComponent {
	char *string;
};

COMPONENT_IMPL(TestComponent)
void TestComponent_new(TestComponent *comp) {
	comp->string = malloc(15);
	if (comp->string) strcpy(comp->string, "This is a test.");
}

void TestComponent_free(TestComponent *comp) {
	if (comp->string) free(comp->string);
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
