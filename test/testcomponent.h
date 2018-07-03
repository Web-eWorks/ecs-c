// test/testcomponent.h
#ifndef ECS_TESTCOMPONENT_H
#define ECS_TESTCOMPONENT_H

COMPONENT(TestComponent)
struct TestComponent {
	char *string;
};

const char* TestComponent_GetString(TestComponent *c);

#endif /* end of include guard: ECS_TESTCOMPONENT_H */
