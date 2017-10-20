//
// Created by Martin Kleinhans on 21.08.17.
//

#include "../bgjs/BGJSV8Engine.h"
using namespace v8;

#include "JNIV8Wrapper.h"
#include "JNIV8Array.h"
#include "JNIV8GenericObject.h"
#include "JNIV8Function.h"

#include <string>
#include <algorithm>

std::map<std::string, V8ClassInfoContainer*> JNIV8Wrapper::_objmap;

//const char* JNIV8Wrapper::_v8PrivateKey = "JNIV8WrapperPrivate";

decltype(JNIV8Wrapper::_jniV8FunctionInfo) JNIV8Wrapper::_jniV8FunctionInfo;
decltype(JNIV8Wrapper::_jniV8AccessorInfo) JNIV8Wrapper::_jniV8AccessorInfo;
decltype(JNIV8Wrapper::_jniDouble) JNIV8Wrapper::_jniDouble;
decltype(JNIV8Wrapper::_jniBoolean) JNIV8Wrapper::_jniBoolean;
decltype(JNIV8Wrapper::_jniString) JNIV8Wrapper::_jniString;
decltype(JNIV8Wrapper::_jniCharacter) JNIV8Wrapper::_jniCharacter;
decltype(JNIV8Wrapper::_jniNumber) JNIV8Wrapper::_jniNumber;
decltype(JNIV8Wrapper::_jniV8Object) JNIV8Wrapper::_jniV8Object;
jobject JNIV8Wrapper::_undefined = nullptr;

void JNIV8Wrapper::init() {
    JNIWrapper::registerObject<JNIV8Object>(JNIObjectType::kAbstract);
    _registerObject(JNIV8ObjectType::kAbstract, JNIWrapper::getCanonicalName<JNIV8Object>(), "",
                    nullptr, createJavaClass<JNIV8Object>, sizeof(JNIV8Object));

    JNIV8Wrapper::registerObject<JNIV8Array>(JNIV8ObjectType::kWrapper);
    JNIV8Wrapper::registerObject<JNIV8GenericObject>(JNIV8ObjectType::kWrapper);
    JNIV8Wrapper::registerObject<JNIV8Function>(JNIV8ObjectType::kWrapper);

    JNIEnv *env = JNIWrapper::getEnvironment();

    jclass clsUndefined = env->FindClass("ag/boersego/bgjs/JNIV8Undefined");
    jmethodID getInstanceId = env->GetStaticMethodID(clsUndefined, "GetInstance", "()Lag/boersego/bgjs/JNIV8Undefined;");

    _undefined = env->NewGlobalRef(env->CallStaticObjectMethod(clsUndefined, getInstanceId));

    _jniV8FunctionInfo.clazz = (jclass)env->NewGlobalRef(env->FindClass("ag/boersego/v8annotations/generated/V8FunctionInfo"));
    _jniV8FunctionInfo.propertyId = env->GetFieldID(_jniV8FunctionInfo.clazz, "property", "Ljava/lang/String;");
    _jniV8FunctionInfo.methodId = env->GetFieldID(_jniV8FunctionInfo.clazz, "method", "Ljava/lang/String;");
    _jniV8FunctionInfo.isStaticId = env->GetFieldID(_jniV8FunctionInfo.clazz, "isStatic", "Z");

    _jniV8AccessorInfo.clazz = (jclass)env->NewGlobalRef(env->FindClass("ag/boersego/v8annotations/generated/V8AccessorInfo"));
    _jniV8AccessorInfo.propertyId = env->GetFieldID(_jniV8AccessorInfo.clazz, "property", "Ljava/lang/String;");
    _jniV8AccessorInfo.getterId = env->GetFieldID(_jniV8AccessorInfo.clazz, "getter", "Ljava/lang/String;");
    _jniV8AccessorInfo.setterId = env->GetFieldID(_jniV8AccessorInfo.clazz, "setter", "Ljava/lang/String;");
    _jniV8AccessorInfo.isStaticId = env->GetFieldID(_jniV8AccessorInfo.clazz, "isStatic", "Z");

    _jniDouble.clazz = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Double"));
    _jniDouble.valueOfId = env->GetStaticMethodID(_jniDouble.clazz, "valueOf","(D)Ljava/lang/Double;");

    _jniBoolean.clazz = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Boolean"));
    _jniBoolean.valueOfId = env->GetStaticMethodID(_jniBoolean.clazz, "valueOf","(Z)Ljava/lang/Boolean;");
    _jniBoolean.booleanValueId = env->GetMethodID(_jniBoolean.clazz, "booleanValue","()Z");

    _jniString.clazz = (jclass)env->NewGlobalRef(env->FindClass("java/lang/String"));

    _jniCharacter.clazz = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Character"));
    _jniCharacter.charValueId = env->GetMethodID(_jniCharacter.clazz, "charValue","()C");

    _jniNumber.clazz = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Number"));
    _jniNumber.doubleValueId = env->GetMethodID(_jniNumber.clazz, "doubleValue","()D");

    _jniV8Object.clazz = (jclass)env->NewGlobalRef(env->FindClass("ag/boersego/bgjs/JNIV8Object"));
}

void JNIV8Wrapper::v8ConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::External> ext = args.Data().As<v8::External>();
    V8ClassInfo* info = static_cast<V8ClassInfo*>(ext->Value());

    v8::Isolate* isolate = info->engine->getIsolate();
    HandleScope scope(isolate);

    // check if class can be created from JS
    if(info->createFromNativeOnly) {
        args.GetIsolate()->ThrowException(String::NewFromUtf8(isolate, "Illegal constructor invocation. Instances must not be created from JavaScript"));
        return;
    }

    // create temporary persistent for the js object and then call the constructor
    v8::Persistent<Object>* jsObj = new v8::Persistent<v8::Object>(isolate, args.This());
    auto ptr = info->container->creator(info, jsObj);

    if(info->constructorCallback) {
        (ptr.get()->*(info->constructorCallback))(args);
    }
}

V8ClassInfo* JNIV8Wrapper::_getV8ClassInfo(const std::string& canonicalName, BGJSV8Engine *engine) {
    // find class info container
    auto it = _objmap.find(canonicalName);
    JNI_ASSERT(it != _objmap.end(), "Attempt to retrieve class info for unregistered class");

    // check if class info object already exists for this engine!
    for(auto &it2 : it->second->classInfos) {
        if(it2->engine == engine) {
            return it2;
        }
    }
    // if it was not found we have to create it now & link it with the container
    auto v8ClassInfo = new V8ClassInfo(it->second, engine);
    it->second->classInfos.push_back(v8ClassInfo);

    // initialize class info: template with constructor and general setup created here
    // individual methods and accessors handled by static method on subclass
    v8::Isolate* isolate = engine->getIsolate();
    v8::Locker l(isolate);
    Isolate::Scope isolateScope(isolate);
    HandleScope scope(isolate);
    Context::Scope ctxScope(engine->getContext());

    // v8 class name: canonical name with underscores instead of slashes
    // e.g. ag/boersego/bgjs/Test becomes ag_boersego_bgjs_Test
    std::string strV8ClassName = canonicalName;
    std::replace(strV8ClassName.begin(), strV8ClassName.end(), '/', '_');

    Local<External> data = External::New(isolate, (void*)v8ClassInfo);
    Handle<FunctionTemplate> ft = FunctionTemplate::New(isolate, v8ConstructorCallback, data);
    ft->SetClassName(String::NewFromUtf8(isolate, strV8ClassName.c_str()));

    // inherit from baseclass
    if(v8ClassInfo->container->baseClassInfo) {
        V8ClassInfo *baseInfo = nullptr;
        // base classinfo might not have been initialized yet => do so now!
        _getV8ClassInfo(v8ClassInfo->container->baseClassInfo->canonicalName, engine);
        for(auto &it2 : v8ClassInfo->container->baseClassInfo->classInfos) {
            if(it2->engine == engine) {
                baseInfo = it2;
                break;
            }
        }
        JNI_ASSERT(baseInfo, "Failed to retrieve baseclass info");
        Local<FunctionTemplate> baseFT = Local<FunctionTemplate>::New(isolate, baseInfo->functionTemplate);
        ft->Inherit(baseFT);
    }

    Local<ObjectTemplate> tpl = ft->InstanceTemplate();
    tpl->SetInternalFieldCount(1);

    // store
    v8ClassInfo->functionTemplate.Reset(isolate, ft);

    // if this is a pure java class it might not have an initializer
    if(it->second->initializer) {
        it->second->initializer(v8ClassInfo);
    }

    // but it might have bindings on java that need to be processed
    // binding classes + methods do not need to be cached here, because they are only used once per Engine upon initialization!
    JNIEnv *env = JNIWrapper::getEnvironment();
    jclass clsObject = env->FindClass(canonicalName.c_str());
    jclass clsBinding = env->FindClass((canonicalName+"V8Binding").c_str());
    if(clsBinding && clsObject) {
        jfieldID createFromNativeOnlyId = env->GetStaticFieldID(clsBinding, "createFromNativeOnly", "Z");
        v8ClassInfo->createFromNativeOnly = env->GetStaticBooleanField(clsBinding, createFromNativeOnlyId);

        jmethodID getFunctionsMethodId = env->GetStaticMethodID(clsBinding, "getV8Functions",
                                                                "()[Lag/boersego/v8annotations/generated/V8FunctionInfo;");
        jmethodID getAccessorsMethodId = env->GetStaticMethodID(clsBinding, "getV8Accessors",
                                                                "()[Lag/boersego/v8annotations/generated/V8AccessorInfo;");

        jobjectArray functionInfos = (jobjectArray) env->CallStaticObjectMethod(clsBinding,
                                                                                getFunctionsMethodId);
        for(jsize idx=0,n=env->GetArrayLength(functionInfos);idx<n;idx++) {
            jobject functionInfo = env->GetObjectArrayElement(functionInfos, idx);
            const std::string strFunctionName = JNIWrapper::jstring2string((jstring)env->GetObjectField(functionInfo, _jniV8FunctionInfo.propertyId));
            const std::string strMethodName = JNIWrapper::jstring2string((jstring)env->GetObjectField(functionInfo, _jniV8FunctionInfo.methodId));
            jmethodID javaMethodId;
            if(env->GetBooleanField(functionInfo, _jniV8FunctionInfo.isStaticId)) {
                javaMethodId = env->GetStaticMethodID(clsObject, strMethodName.c_str(),
                                                "([Ljava/lang/Object;)Ljava/lang/Object;");
                v8ClassInfo->registerStaticJavaMethod(strFunctionName, javaMethodId);
            } else {
                javaMethodId = env->GetMethodID(clsObject, strMethodName.c_str(),
                                                "([Ljava/lang/Object;)Ljava/lang/Object;");
                v8ClassInfo->registerJavaMethod(strFunctionName, javaMethodId);
            }
        }

        jobjectArray accessorInfos = (jobjectArray) env->CallStaticObjectMethod(clsBinding,
                                                                                getAccessorsMethodId);
        for(jsize idx=0,n=env->GetArrayLength(accessorInfos);idx<n;idx++) {
            jobject accessorInfo = env->GetObjectArrayElement(accessorInfos, idx);
            const std::string strPropertyName = JNIWrapper::jstring2string((jstring)env->GetObjectField(accessorInfo, _jniV8AccessorInfo.propertyId));
            const std::string strGetterName = JNIWrapper::jstring2string((jstring)env->GetObjectField(accessorInfo, _jniV8AccessorInfo.getterId));
            const std::string strSetterName = JNIWrapper::jstring2string((jstring)env->GetObjectField(accessorInfo, _jniV8AccessorInfo.setterId));
            jmethodID javaGetterId, javaSetterId;
            if(env->GetBooleanField(accessorInfo, _jniV8AccessorInfo.isStaticId)) {
                javaGetterId = env->GetStaticMethodID(clsObject, strGetterName.c_str(), "()Ljava/lang/Object;");
                javaSetterId = env->GetStaticMethodID(clsObject, strSetterName.c_str(), "(Ljava/lang/Object;)V");
                v8ClassInfo->registerStaticJavaAccessor(strPropertyName, javaGetterId, javaSetterId);
            } else {
                javaGetterId = env->GetMethodID(clsObject, strGetterName.c_str(), "()Ljava/lang/Object;");
                javaSetterId = env->GetMethodID(clsObject, strSetterName.c_str(), "(Ljava/lang/Object;)V");
                v8ClassInfo->registerJavaAccessor(strPropertyName, javaGetterId, javaSetterId);
            }
        }
    } else {
        env->ExceptionClear();
    }

    return v8ClassInfo;
}

void JNIV8Wrapper::initializeNativeJNIV8Object(jobject obj, jlong enginePtr, jlong jsObjPtr) {
    auto v8Object = JNIWrapper::wrapObject<JNIV8Object>(obj);
    BGJSV8Engine *engine = reinterpret_cast<BGJSV8Engine*>(enginePtr);
    V8ClassInfo *classInfo = JNIV8Wrapper::_getV8ClassInfo(v8Object->getCanonicalName(), engine);

    v8::Isolate* isolate = engine->getIsolate();
    v8::Locker l(isolate);
    Isolate::Scope isolateScope(isolate);
    HandleScope scope(isolate);
    Context::Scope ctxScope(engine->getContext());

    v8::Persistent<Object>* persistentPtr;
    v8::Local<Object> jsObj;

    // if an object was already supplied we just need to extract it and store it
    if(jsObjPtr) {
        persistentPtr = reinterpret_cast<v8::Persistent<Object>*>(jsObjPtr);
        jsObj = v8::Local<Object>::New(isolate, *persistentPtr);
        // clear and delete persistent
        persistentPtr->Reset();
        delete persistentPtr;
    }

    // adjust external memory size: size of native object + 64bit pointer to native object stored on java
    v8Object->adjustJSExternalMemory(classInfo->container->size + 8);

    // associate js object with native c++ object
    v8Object->setJSObject(engine, classInfo, jsObj);
}

void JNIV8Wrapper::_registerObject(JNIV8ObjectType type, const std::string& canonicalName, const std::string& baseCanonicalName, JNIV8ObjectInitializer i, JNIV8ObjectCreator c, size_t size) {
    // canonicalName may be already registered
    // (e.g. when called from JNI_OnLoad; when using multiple linked libraries it is called once for each library)
    if (_objmap.find(canonicalName) != _objmap.end()) {
        return;
    }

    // base class has to be registered if it is not JNIV8Object (which is only registered with JNIWrapper, because it provides no JS functionality on its own)
    V8ClassInfoContainer *baseInfo = nullptr;
    if (!baseCanonicalName.empty()) {
        auto it = _objmap.find(baseCanonicalName);
        if (it == _objmap.end()) {
            return;
        }
        baseInfo = it->second;
    } else if(canonicalName != JNIWrapper::getCanonicalName<JNIV8Object>()) {
        // an empty base class is only allowed here for internally registering JNIObject itself
        JNI_ASSERT(0, "Attempt to register an object without super class");
        return;
    }

    if(baseInfo) {
        if (type == JNIV8ObjectType::kWrapper) {
            // wrapper classes can only extend other wrapper classes (or JNIV8Object directly)
            V8ClassInfoContainer *baseInfo2 = baseInfo;
            do {
                if (baseInfo2->type != JNIV8ObjectType::kWrapper && baseInfo2->baseClassInfo) {
                    return;
                }
                baseInfo2 = baseInfo->baseClassInfo;
            } while (baseInfo2);
        } else if (baseInfo->type == JNIV8ObjectType::kWrapper &&
                   baseInfo->type != type) {
            // wrapper classes can only be extended by other wrapper classes!
            return;
        }
    }

    V8ClassInfoContainer *info = new V8ClassInfoContainer(type, canonicalName, i, c, size, baseInfo);
    _objmap[canonicalName] = info;
}

/**
 * convert a jstring to a std::string
 */
v8::Local<v8::String> JNIV8Wrapper::jstring2v8string(jstring string) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    // because this method returns a local, we can assume that the correct v8 scopes are active around it already
    // we still need a handle scope however...
    v8::EscapableHandleScope scope(isolate);
    return scope.Escape(v8::String::NewFromUtf8(isolate, JNIWrapper::jstring2string(string).c_str()));
}

/**
 * convert a std::string to a jstring
 */
jstring JNIV8Wrapper::v8string2jstring(v8::Local<v8::String> string) {
    return JNIWrapper::string2jstring(BGJS_STRING_FROM_V8VALUE(string));
}

/**
 * convert an instance of V8Value to a jobject
 */
jobject JNIV8Wrapper::v8value2jobject(Local<Value> valueRef) {
    JNIEnv *env = JNIWrapper::getEnvironment();

    if(valueRef->IsUndefined()) {
        return _undefined;
    } else if(valueRef->IsNumber()) {
        return env->CallStaticObjectMethod(_jniDouble.clazz, _jniDouble.valueOfId, valueRef->NumberValue());
    } else if(valueRef->IsString()) {
        return JNIWrapper::string2jstring(BGJS_STRING_FROM_V8VALUE(valueRef));
    } else if(valueRef->IsBoolean()) {
        return env->CallStaticObjectMethod(_jniBoolean.clazz, _jniBoolean.valueOfId, valueRef->BooleanValue());
    } else if(valueRef->IsObject()) {
        if(valueRef->IsFunction()){
            return JNIV8Wrapper::wrapObject<JNIV8Function>(valueRef->ToObject())->getJObject();
        } else if(valueRef->IsArray()){
            return JNIV8Wrapper::wrapObject<JNIV8Array>(valueRef->ToObject())->getJObject();
        } else if(valueRef->IsNull()) {
            return nullptr;
        }
        auto ptr = JNIV8Wrapper::wrapObject<JNIV8Object>(valueRef->ToObject());
        if(ptr) {
            return ptr->getJObject();
        } else {
            return JNIV8Wrapper::wrapObject<JNIV8GenericObject>(valueRef->ToObject())->getJObject();
        }
    } else if(valueRef->IsSymbol()) {
        JNI_ASSERT(0, "Symbols are not supported"); // return env->NewObject(clsJNIV8Value, constructor, 4, nullptr);
    } else {
        JNI_ASSERT(0, "Encountered unexpected v8 type");
    }
    return nullptr;
}

/**
 * convert an instance of Object to a v8value
 */
v8::Local<v8::Value> JNIV8Wrapper::jobject2v8value(jobject object) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    // because this method returns a local, we can assume that the correct v8 scopes are active around it already
    // we still need a handle scope however...
    v8::EscapableHandleScope scope(isolate);

    v8::Local<v8::Value> resultRef;

    JNIEnv *env = JNIWrapper::getEnvironment();

    // jobject referencing "null" can actually be non-null..
    if(env->IsSameObject(object, NULL) || !object) {
        resultRef = v8::Null(isolate);
    } else if(env->IsInstanceOf(object, _jniString.clazz)) {
        resultRef = JNIV8Wrapper::jstring2v8string((jstring)object);
    } else if(env->IsInstanceOf(object, _jniCharacter.clazz)) {
        jchar c = env->CallCharMethod(object, _jniCharacter.charValueId);
        v8::MaybeLocal<v8::String> maybeLocal = v8::String::NewFromTwoByte(isolate, &c, NewStringType::kNormal, 1);
        if(!maybeLocal.IsEmpty()) {
            resultRef = maybeLocal.ToLocalChecked();
        }
    } else if(env->IsInstanceOf(object, _jniNumber.clazz)) {
        jdouble n = env->CallDoubleMethod(object, _jniNumber.doubleValueId);
        resultRef = v8::Number::New(isolate, n);
    } else if(env->IsInstanceOf(object, _jniBoolean.clazz)) {
        jboolean b = env->CallBooleanMethod(object, _jniBoolean.booleanValueId);
        resultRef = v8::Boolean::New(isolate, b);
    } else if(env->IsInstanceOf(object, _jniV8Object.clazz)) {
        resultRef = JNIV8Wrapper::wrapObject<JNIV8Object>(object)->getJSObject();
    }
    if(resultRef.IsEmpty()) {
        resultRef = v8::Undefined(isolate);
    }
    return scope.Escape(resultRef);
}

// persistent classes can also be accessed as JNIV8Object directly!
template <> std::shared_ptr<JNIV8Object> JNIV8Wrapper::wrapObject<JNIV8Object>(v8::Local<v8::Object> object) {
    v8::Local<v8::External> ext;
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    // because this method takes a local, we can be sure that the correct v8 scopes are active around it already
    // we still need a handle scope however...
    v8::HandleScope scope(isolate);

    if (object->InternalFieldCount() >= 1) {
        // does the object have internal fields? if so use it!
        ext = object->GetInternalField(0).As<v8::External>();
    } else {
        return nullptr;
    }
    return std::static_pointer_cast<JNIV8Object>(reinterpret_cast<JNIV8Object*>(ext->Value())->getSharedPtr());
};

/**
 * internal helper function called by V8Engine on destruction
 */
void JNIV8Wrapper::cleanupV8Engine(BGJSV8Engine *engine) {
    for(auto it : _objmap) {
        for (auto it2 = it.second->classInfos.begin(); it2 != it.second->classInfos.end(); ++it2) {
            if((*it2)->engine == engine) {
                delete *it2;
                it.second->classInfos.erase(it2);
                break;
            }
        }
    }
}

/**
 * return an object representing undefined in java
 */
jobject JNIV8Wrapper::undefinedInJava() {
    return _undefined;
}