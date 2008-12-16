#include <jvmti.h>
#include <stdio.h>

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("AGENT: Agent_OnLoad\n");

    return 0;
}
