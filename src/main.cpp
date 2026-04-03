#include <jni.h>
#include <jvmti.h>
#include <print>
#include <thread>

#include "mc_internal/core/bootstrap.hpp"
#include "mc_internal/utils/logging.hpp"

static void JNICALL OnVmInit(jvmtiEnv* jvmti_env, JNIEnv* jni_env, jthread thread) {
  static_cast<void>(jvmti_env);
  static_cast<void>(thread);

  JavaVM* vm = nullptr;
  if (jni_env->GetJavaVM(&vm) != JNI_OK || vm == nullptr) {
    mc_internal::PrintStatus("vm init callback could not fetch the java vm");
    return;
  }

  try {
    std::thread{mc_internal::Bootstrap, vm}.detach();
  } catch (const std::exception& exception) {
    std::println(
        "{} bootstrap thread launch failed: {}", mc_internal::kLogPrefix, exception.what());
  }
}

extern "C" JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM* vm, char* options, void* reserved) {
  static_cast<void>(options);
  static_cast<void>(reserved);

  mc_internal::PrintStatus("jvmti agent loaded natively");

  jvmtiEnv* jvmti = nullptr;
  if (vm->GetEnv(reinterpret_cast<void**>(&jvmti), JVMTI_VERSION_1_2) != JNI_OK ||
      jvmti == nullptr) {
    mc_internal::PrintStatus("could not acquire jvmti env during onload");
    return JNI_ERR;
  }

  jvmtiEventCallbacks callbacks{};
  callbacks.VMInit = &OnVmInit;

  if (jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)) != JVMTI_ERROR_NONE) {
    mc_internal::PrintStatus("could not install jvmti callbacks");
    return JNI_ERR;
  }

  if (jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, nullptr) !=
      JVMTI_ERROR_NONE) {
    mc_internal::PrintStatus("could not subscribe to vm init");
    return JNI_ERR;
  }

  return JNI_OK;
}
