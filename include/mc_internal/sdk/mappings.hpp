#pragma once

// Fabric intermediary names for Minecraft 1.21.11.

#include <string_view>

namespace mc_internal {

inline constexpr std::string_view kMinecraftClientClass = "net/minecraft/class_310";
inline constexpr std::string_view kEntityClass = "net/minecraft/class_1297";
inline constexpr std::string_view kClientPlayerEntityClass = "net/minecraft/class_746";
inline constexpr std::string_view kClientWorldClass = "net/minecraft/class_638";
inline constexpr std::string_view kGameRendererClass = "net/minecraft/class_757";
inline constexpr std::string_view kCameraClass = "net/minecraft/class_4184";
inline constexpr std::string_view kVec3dClass = "net/minecraft/class_243";
inline constexpr std::string_view kJomlMatrix4fClass = "org/joml/Matrix4f";
inline constexpr std::string_view kEntityTypeClass = "net/minecraft/class_1299";
inline constexpr std::string_view kHostileEntityClass = "net/minecraft/class_1588";
inline constexpr std::string_view kPassiveEntityClass = "net/minecraft/class_1296";
inline constexpr std::string_view kItemEntityClass = "net/minecraft/class_1542";

inline constexpr std::string_view kJavaLangIterableClass = "java/lang/Iterable";
inline constexpr std::string_view kJavaUtilIteratorClass = "java/util/Iterator";
inline constexpr std::string_view kIterableIteratorMethod = "iterator";
inline constexpr std::string_view kIterableIteratorSignature = "()Ljava/util/Iterator;";
inline constexpr std::string_view kIteratorHasNextMethod = "hasNext";
inline constexpr std::string_view kIteratorHasNextSignature = "()Z";
inline constexpr std::string_view kIteratorNextMethod = "next";
inline constexpr std::string_view kIteratorNextSignature = "()Ljava/lang/Object;";

inline constexpr std::string_view kMinecraftClientGetInstanceMethod = "method_1551";
inline constexpr std::string_view kMinecraftClientGetInstanceSignature =
    "()Lnet/minecraft/class_310;";

inline constexpr std::string_view kMinecraftClientPlayerField = "field_1724";
inline constexpr std::string_view kMinecraftClientPlayerFieldSignature =
    "Lnet/minecraft/class_746;";

inline constexpr std::string_view kMinecraftClientWorldField = "field_1687";
inline constexpr std::string_view kMinecraftClientWorldFieldSignature = "Lnet/minecraft/class_638;";

inline constexpr std::string_view kMinecraftClientGameRendererField = "field_1773";
inline constexpr std::string_view kMinecraftClientGameRendererSignature =
    "Lnet/minecraft/class_757;";
inline constexpr std::string_view kMinecraftClientGetRenderTickCounterMethod = "method_61966";
inline constexpr std::string_view kMinecraftClientGetRenderTickCounterSignature =
    "()Lnet/minecraft/class_9779;";
inline constexpr std::string_view kRenderTickCounterClass = "net/minecraft/class_9779";
inline constexpr std::string_view kRenderTickCounterGetTickProgressMethod = "method_60637";
inline constexpr std::string_view kRenderTickCounterGetTickProgressSignature = "(Z)F";

inline constexpr std::string_view kClientWorldGetEntitiesMethod = "method_18112";
inline constexpr std::string_view kClientWorldGetEntitiesSignature = "()Ljava/lang/Iterable;";

inline constexpr std::string_view kEntityGetXMethod = "method_23317";
inline constexpr std::string_view kEntityGetYMethod = "method_23318";
inline constexpr std::string_view kEntityGetZMethod = "method_23321";
inline constexpr std::string_view kEntityGetCoordSignature = "()D";
inline constexpr std::string_view kEntityIsAliveMethod = "method_5805";
inline constexpr std::string_view kEntityIsAliveSignature = "()Z";
inline constexpr std::string_view kEntityGetLastRenderPosMethod = "method_61411";
inline constexpr std::string_view kEntityGetLastRenderPosSignature = "()Lnet/minecraft/class_243;";
inline constexpr std::string_view kEntityGetHeightMethod = "method_17682";
inline constexpr std::string_view kEntityGetHeightSignature = "()F";
inline constexpr std::string_view kEntityGetTypeMethod = "method_5864";
inline constexpr std::string_view kEntityGetTypeSignature = "()Lnet/minecraft/class_1299;";

inline constexpr std::string_view kEntityTypeTranslationKeyField = "field_6106";
inline constexpr std::string_view kEntityTypeTranslationKeyFieldSignature = "Ljava/lang/String;";

inline constexpr std::string_view kGameRendererGetCameraMethod = "method_19418";
inline constexpr std::string_view kGameRendererGetCameraSignature = "()Lnet/minecraft/class_4184;";
inline constexpr std::string_view kGameRendererGetFovMethod = "method_3196";
inline constexpr std::string_view kGameRendererGetFovSignature = "(Lnet/minecraft/class_4184;FZ)F";

inline constexpr std::string_view kCameraPosField = "field_18712";
inline constexpr std::string_view kCameraPosSignature = "Lnet/minecraft/class_243;";
inline constexpr std::string_view kCameraGetPitchMethod = "method_19329";
inline constexpr std::string_view kCameraGetPitchSignature = "()F";
inline constexpr std::string_view kCameraGetYawMethod = "method_19330";
inline constexpr std::string_view kCameraGetYawSignature = "()F";

inline constexpr std::string_view kVec3dGetXField = "field_1352";
inline constexpr std::string_view kVec3dGetYField = "field_1351";
inline constexpr std::string_view kVec3dGetZField = "field_1350";
inline constexpr std::string_view kVec3dCoordSignature = "D";

}  // namespace mc_internal
