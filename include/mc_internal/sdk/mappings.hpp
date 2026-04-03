#pragma once
#include <string_view>

namespace mc_internal {

inline constexpr std::string_view kMinecraftClientClass = "net/minecraft/class_310";
inline constexpr std::string_view kEntityClass = "net/minecraft/class_1297";
inline constexpr std::string_view kClientPlayerEntityClass = "net/minecraft/class_746";
inline constexpr std::string_view kClientWorldClass = "net/minecraft/class_638";

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

inline constexpr std::string_view kClientWorldGetEntitiesMethod = "";
inline constexpr std::string_view kClientWorldGetEntitiesSignature = "()Ljava/lang/Iterable;";

inline constexpr std::string_view kEntityGetXMethod = "method_23317";
inline constexpr std::string_view kEntityGetYMethod = "method_23318";
inline constexpr std::string_view kEntityGetZMethod = "method_23321";
inline constexpr std::string_view kEntityGetCoordSignature = "()D";

inline constexpr std::string_view kGameRendererClass = "net/minecraft/class_315";
inline constexpr std::string_view kCameraClass = "net/minecraft/class_4184";
inline constexpr std::string_view kJomlMatrix4fClass = "org/joml/Matrix4f";

}  // namespace mc_internal
