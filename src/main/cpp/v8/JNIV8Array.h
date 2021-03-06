//
// Created by Martin Kleinhans on 26.09.17.
//

#ifndef ANDROID_TRADINGLIB_SAMPLE_JNIV8ARRAY_H
#define ANDROID_TRADINGLIB_SAMPLE_JNIV8ARRAY_H

#include "JNIV8Wrapper.h"

class JNIV8Array : public JNIScope<JNIV8Array, JNIV8Object> {
public:
    JNIV8Array(jobject obj, JNIClassInfo *info) : JNIScope(obj, info) {};

    static void initializeJNIBindings(JNIClassInfo *info, bool isReload);
    static void initializeV8Bindings(JNIV8ClassInfo *info);

    static jobject jniCreate(JNIEnv *env, jobject obj, jobject engineObj);
    static jobject jniCreateWithLength(JNIEnv *env, jobject obj, jobject engineObj, jint length);
    static jobject jniCreateWithArray(JNIEnv *env, jobject obj, jobject engineObj, jobjectArray elements);

    /**
     * returns the length of the array
     */
    static jint jniGetV8Length(JNIEnv *env, jobject obj);

    /**
     * Returns all objects from a specified range inside of the array
     */
    static jobjectArray jniGetV8ElementsInRange(JNIEnv *env, jobject obj, jint flags, jint type, jclass returnType, jint from, jint to);

    /**
     * Returns the object at the specified index
     * if index is out of bounds, returns JNIV8Undefined
     */
    static jobject jniGetV8Element(JNIEnv *env, jobject obj, jint flags, jint type, jclass returnType, jint index);

    /**
     * cache JNI class references
     */
    static void initJNICache();
private:
    static struct {
        jclass clazz;
    } _jniObject;
};

BGJS_JNI_LINK_DEF(JNIV8Array)

#endif //ANDROID_TRADINGLIB_SAMPLE_JNIV8ARRAY_H
