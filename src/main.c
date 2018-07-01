#include <stdio.h>
#include "ecs.h"

struct TestComponent {
	const char *string;
};


int main (int argc, const char **argv) {

	const char *str = "This is a test.";
	hash_t str_hash = hash_string(str);

	printf("%s -> %x\n", str, str_hash);
	
	ECS *ecs = ECS_New();
	
	bool res = ECS_ComponentRegisterType(
		ecs,
		"TestComponent",
		NULL, NULL,
		sizeof(struct TestComponent)
	);
		
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
