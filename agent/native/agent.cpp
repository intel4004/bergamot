#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jvmti.h>

#define ENABLE_TRACING

#ifdef ENABLE_TRACING
  #define TRACE(format, args...) fprintf (stderr, "TRACE: "format"\n" , ## args)
#else
  #define TRACE(format, args...)
#endif

static jvmtiEnv* jvmti;

static void JNICALL
VMInit(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    TRACE("VMInit");
}

void JNICALL
VMStart(jvmtiEnv *jvmti_env, JNIEnv* jni_env) {
    TRACE("VMStart");
}

void JNICALL
VMDeath(jvmtiEnv *jvmti_env, JNIEnv* jni_env) {
    TRACE("VMDeath");
}

void JNICALL
ThreadStart(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
    TRACE("ThreadStart");
}

void JNICALL
ThreadEnd(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread) {
    TRACE("ThreadEnd");
}

void JNICALL
ClassFileLoadHook(
    jvmtiEnv *jvmti_env,
    JNIEnv* jni_env,
    jclass class_being_redefined,
    jobject loader,
    const char* name,
    jobject protection_domain,
    jint class_data_len,
    const unsigned char* class_data,
    jint* new_class_data_len,
    unsigned char** new_class_data)
{
    TRACE("ClassFileLoadHook %s", name);
    
}

void JNICALL
GarbageCollectionStart(jvmtiEnv *jvmti_env) {
    TRACE("GarbageCollectionStart");
}

void JNICALL
GarbageCollectionFinish(jvmtiEnv *jvmti_env) {
    TRACE("GarbageCollectionFinish");
}

void JNICALL
MonitorContendedEnter(jvmtiEnv *jvmti_env,JNIEnv* jni_env, jthread thread, jobject object) {
    TRACE("MonitorContendedEnter");
}

void JNICALL
MonitorContendedEntered(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jobject object) {
    TRACE("MonitorContendedEntered");
}

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    TRACE("Agent_OnLoad");

    jint result;
    result = vm->GetEnv((void**)&jvmti, JVMTI_VERSION);
    if (result != JNI_OK) {
	fprintf(stderr, "ERROR: Unable to create jvmtiEnv, GetEnv failed with error code %d\n", result);
	return -1;
    }


    jvmtiCapabilities capabilities;
    result = jvmti->GetCapabilities(&capabilities);
    if (result != JVMTI_ERROR_NONE) {
	fprintf(stderr, "ERROR: GetCapabilities failed with error code %d\n", result);
    }
    capabilities.can_generate_monitor_events = 1;
    capabilities.can_generate_garbage_collection_events = 1;
    result = jvmti->AddCapabilities(&capabilities);
    if (result != JVMTI_ERROR_NONE) {
	fprintf(stderr, "ERROR: AddCapabilities failed with error code %d\n", result);
	return -1;
    }

    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &VMInit;
    callbacks.VMStart = &VMStart;
    callbacks.VMDeath = &VMDeath;
    callbacks.ThreadStart = &ThreadStart;
    callbacks.ThreadEnd = &ThreadEnd;
    callbacks.ClassFileLoadHook = &ClassFileLoadHook;
    callbacks.GarbageCollectionStart = &GarbageCollectionStart;
    callbacks.GarbageCollectionFinish = &GarbageCollectionFinish;
    callbacks.MonitorContendedEnter = &MonitorContendedEnter;
    callbacks.MonitorContendedEntered = &MonitorContendedEntered;
    jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER, NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, NULL);


    return 0;
}
