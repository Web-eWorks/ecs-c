#include <stdio.h>
#include <string.h>
#include "ecs.h"

COMPONENT(TestComponent)
struct TestComponent {
	char *string;
};

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
	
	Entity *entity = ECS_EntityNew(ecs);
	printf("%p.\n", entity);
	ECS_AddEntity(ecs, entity);
	
	printf("Part 1/2 done.\n");
	
	ECS_RemoveEntity(ecs, entity);
	ECS_EntityDelete(entity);
	
	ECS_Delete(ecs);
	
	printf("Part 2/2 done.\n");

	return 0;
}
