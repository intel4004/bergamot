#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>
#include <time.h>

#include <jvmti.h>

//#define ENABLE_TRACING

#define ERROR(format, args...) fprintf (stderr, "ERROR: "format"\n" , ## args)

#ifdef ENABLE_TRACING
  #define TRACE(format, args...) fprintf (stderr, "TRACE: "format"\n" , ## args)
#else
  #define TRACE(format, args...)
#endif

static JavaVM *jvm;
static jvmtiEnv* jvmti;


long long getNanoTime() {
    struct timespec currentTime;
    if (clock_gettime( CLOCK_REALTIME, &currentTime ) == 0) {
        return currentTime.tv_sec * 1000000000L + currentTime.tv_nsec;
    }
    return 0;
}

int usleep(long usec) {
    struct timeval tv;
    tv.tv_sec = usec / 1000000L;
    tv.tv_usec = usec % 1000000L;
    return select(0, 0, 0, 0, &tv);
}

#define MAX_FRAMES 256

jint* index_buffer = new jint[1];
jint index_size = 1;
jint index_count = 0;

void appendIndex(jint thread) {
    if(index_count == index_size) {
        jint* old = index_buffer;
        index_buffer = new jint[index_size * 2];
        index_size = index_size * 2;
        for(int i = 0; i < index_count; i++) {
            index_buffer[i] = old[i];
        }
        delete[] old;

        TRACE("Growing thread buffer to %d", index_size);
    }
    index_buffer[index_count++] = thread;
}

jmethodID* method_buffer = new jmethodID[1];
jint method_size = 1;
jint method_count = 0;

void appendMethod(jmethodID method) {
    if(method_count == method_size) {
        jmethodID* old = method_buffer;
        method_buffer = new jmethodID[method_size * 2];
        method_size = method_size * 2;
        for(int i = 0; i < method_count; i++) {
            method_buffer[i] = old[i];
        }
        delete[] old;

        TRACE("Growing method buffer to %d", method_size);
    }
    method_buffer[method_count++] = method;
}

long long total = 0;
long long count = 0;

void recordStackDump(jvmtiEnv* jvmti) {

    //long long before = getNanoTime();
    
    jvmtiStackInfo* stackInfo;
    jint threadCount;
    jvmtiError err;

    err = jvmti->GetAllStackTraces(MAX_FRAMES, &stackInfo, &threadCount);
    if (err != JVMTI_ERROR_NONE) {
        ERROR("GetAllStackTraces failed with %d", err);
        return;
    }
    
    for (int ti = 0; ti < threadCount; ++ti) {
        jvmtiStackInfo* infop = &stackInfo[ti];
        jthread thread = infop->thread;
        jvmtiFrameInfo* frames = infop->frame_buffer;

        jvmtiThreadInfo thread_info;
        err = jvmti->GetThreadInfo(thread, &thread_info);
        if(err != JVMTI_ERROR_NONE) {
           ERROR("GetMethodName failed with %d", err);
           return;
        }

        if(infop->frame_count > 0) {
            appendIndex(method_count);
            for (int fi = 0; fi < infop->frame_count; fi++) {
                appendMethod(frames[fi].method);
            }
        }
    }
    
    err = jvmti->Deallocate((unsigned char*) stackInfo);    
    
    //long long after =  getNanoTime();
    //total += after-before;
    //count++;
    //fprintf (stderr, "time = %lldns (%lldns avg)\n", after-before, total / count);
}

void dumpRecordedStackDumps(jvmtiEnv* jvmti) {
    
    jvmtiError err;

    for(int i = 0; i < index_count; i++) {
        int start = index_buffer[i];
        int end = i == index_count - 1 ? method_count : index_buffer[i + 1];
        for(int j = start; j < end; j++) {
            char* name_ptr = NULL;
            char* method_signature_ptr = NULL;
            char* method_generic_ptr = NULL;

            err = jvmti->GetMethodName(method_buffer[j], &name_ptr, &method_signature_ptr, &method_generic_ptr);
            if(err !=  JVMTI_ERROR_NONE) {
                ERROR("GetMethodName failed with %d", err);
                break;
            }

            jclass klass;
            err = jvmti->GetMethodDeclaringClass(method_buffer[j],  &klass);
            if(err != JVMTI_ERROR_NONE) {
               ERROR("GetMethodDeclaringClass failed with %d", err);
               break;
            }

            char* class_signature_ptr;
            char* class_generic_ptr;
            err = jvmti->GetClassSignature(klass, &class_signature_ptr, &class_generic_ptr);
            if(err != JVMTI_ERROR_NONE) {
                ERROR("GetClassSignature failed with %d", err);
                break;
            }

            char* source_name_ptr;
            err = jvmti->GetSourceFileName(klass, &source_name_ptr);
            if(err != JVMTI_ERROR_NONE) {
                source_name_ptr = "<unknown>";
            }

            fprintf (stderr, "%s %s %s %s\n", class_signature_ptr, name_ptr, method_signature_ptr, source_name_ptr);
        }
        fprintf (stderr, "\n");
    }
}

void printStackDump(jvmtiEnv* jvmti) {

    jvmtiStackInfo* stackInfo;
    jint threadCount;
    jvmtiError err;

    err = jvmti->GetAllStackTraces(MAX_FRAMES, &stackInfo, &threadCount);
    if (err != JVMTI_ERROR_NONE) {
        return;
    }

    for (int ti = 0; ti < threadCount; ++ti) {
        jvmtiStackInfo *infop = &stackInfo[ti];
        jthread thread = infop->thread;
        jint state = infop->state;
        jvmtiFrameInfo *frames = infop->frame_buffer;

        jvmtiThreadInfo thread_info;
        err = jvmti->GetThreadInfo(thread, &thread_info);
        if(err != JVMTI_ERROR_NONE) {
           ERROR("GetMethodName failed with %d", err);
           return;
        }

        fprintf (stderr, "THREAD %s %d\n", thread_info.name, state);
        for (int fi = 0; fi < infop->frame_count; fi++) {

            char* name_ptr = NULL;
            char* method_signature_ptr = NULL;
            char* method_generic_ptr = NULL;

            err = jvmti->GetMethodName(frames[fi].method, &name_ptr, &method_signature_ptr, &method_generic_ptr);
            if(err !=  JVMTI_ERROR_NONE) {
              ERROR("GetMethodName failed with %d", err);
              break;
            }

            jclass klass;
            err = jvmti->GetMethodDeclaringClass(frames[fi].method,  &klass);
            if(err != JVMTI_ERROR_NONE) {
               ERROR("GetMethodDeclaringClass failed with %d", err);
               break;
            }

            char* class_signature_ptr;
            char* class_generic_ptr;
            err = jvmti->GetClassSignature(klass, &class_signature_ptr, &class_generic_ptr);
            if(err != JVMTI_ERROR_NONE) {
                ERROR("GetClassSignature failed with %d", err);
                break;
            }

            char* source_name_ptr;
            err = jvmti->GetSourceFileName(klass, &source_name_ptr);
            if(err != JVMTI_ERROR_NONE) {
                ERROR("GetSourceFileName failed with %d", err);
                break;
            }

            fprintf (stderr, "  FRAME %s %s %s %s:%d\n", class_signature_ptr, name_ptr, method_signature_ptr, source_name_ptr, frames[fi].location);
        }
    }

    err = jvmti->Deallocate((unsigned char*) stackInfo);

}

pthread_t samplingThread;
volatile bool isShutdown;

void* runSamplingThread(void* tid) {
    JNIEnv* jni;
    jint res = jvm->AttachCurrentThreadAsDaemon((void**)&jni, NULL);
    if(res) {
        ERROR("AttachCurrentThreadAsDaemon failed with &d\n", res);
        return NULL;
    }
    while(!isShutdown) {
        recordStackDump(jvmti);
        usleep(250000);
    }

    res = jvm->DetachCurrentThread();
    if(res) {
         ERROR("DetachCurrentThread failed with &d\n", res);
    }

    return NULL;
}

static void JNICALL
VMInit(jvmtiEnv* jvmti, JNIEnv *env, jthread thread) {
    TRACE("VMInit");

    void* t;
    isShutdown = false;
    int result = pthread_create(&samplingThread, NULL, runSamplingThread, t);
    if (result){
        ERROR("pthread_create failed with %d\n", result);
        return;
    }
}

void JNICALL
VMStart(jvmtiEnv* jvmti_env, JNIEnv* jni_env) {
    TRACE("VMStart");
}

void JNICALL
VMDeath(jvmtiEnv* jvmti_env, JNIEnv* jni_env) {    
    TRACE("VMDeath");

    isShutdown = true;
    void* status;
    int result = pthread_join(samplingThread, &status);
    if (result) {
        ERROR("pthread_join failed with %d\n", result);
    }

    dumpRecordedStackDumps(jvmti_env);
}

void JNICALL
ThreadStart(jvmtiEnv* jvmti_env, JNIEnv* jni_env, jthread thread) {
    TRACE("ThreadStart");
}

void JNICALL
ThreadEnd(jvmtiEnv* jvmti_env, JNIEnv* jni_env, jthread thread) {
    TRACE("ThreadEnd");
}

void JNICALL
ClassFileLoadHook(
    jvmtiEnv* jvmti_env,
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
GarbageCollectionStart(jvmtiEnv* jvmti_env) {
    TRACE("GarbageCollectionStart");
}

void JNICALL
GarbageCollectionFinish(jvmtiEnv* jvmti_env) {
    TRACE("GarbageCollectionFinish");
}

void JNICALL
MonitorContendedEnter(jvmtiEnv* jvmti_env,JNIEnv* jni_env, jthread thread, jobject object) {
    TRACE("MonitorContendedEnter");
}

void JNICALL
MonitorContendedEntered(jvmtiEnv* jvmti_env, JNIEnv* jni_env, jthread thread, jobject object) {
    TRACE("MonitorContendedEntered");
}

JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    TRACE("Agent_OnLoad");

    jvm = vm;

    jint result;
    result = vm->GetEnv((void**)&jvmti, JVMTI_VERSION);
    if (result != JNI_OK) {
	fprintf(stderr, "ERROR: Unable to create jvmtiEnv, GetEnv failed with error code %d\n", result);
	return -1;
    }


    jvmtiCapabilities capabilities;
    jvmtiError err;

    err = jvmti->GetCapabilities(&capabilities);
    if (err != JVMTI_ERROR_NONE) {
	fprintf(stderr, "ERROR: GetCapabilities failed with error code %d\n", err);
    }
    capabilities.can_generate_monitor_events = 1;
    capabilities.can_generate_garbage_collection_events = 1;
    capabilities.can_get_source_file_name = 1;


    err = jvmti->AddCapabilities(&capabilities);
    if (err != JVMTI_ERROR_NONE) {
	fprintf(stderr, "ERROR: AddCapabilities failed with error code %d\n", err);
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
