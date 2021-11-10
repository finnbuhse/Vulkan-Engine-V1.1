#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include <string>
#include <cassert>

#define N_DEVICE_FEATURES 55

#ifdef NDEBUG
#define validateResult(result)
#else
#define validateResult(result) \
switch (result) \
{ \
case VK_SUCCESS: break; \
case VK_ERROR_OUT_OF_HOST_MEMORY: assert(("[ERROR] Vulkan out of host memory", false)); \
case VK_ERROR_OUT_OF_DEVICE_MEMORY: assert(("[ERROR] Vulkan out of device memory", false)); \
case VK_ERROR_INITIALIZATION_FAILED: assert(("[ERROR] Vulkan initialization failed", false)); \
case VK_ERROR_DEVICE_LOST: assert(("[ERROR] Vulkan device lost", false)); \
case VK_ERROR_MEMORY_MAP_FAILED: assert(("[ERROR] Vulkan memory map failed", false)); \
case VK_ERROR_LAYER_NOT_PRESENT: assert(("[ERROR] Vulkan layer not present", false)); \
case VK_ERROR_EXTENSION_NOT_PRESENT: assert(("[ERROR] Vulkan extension not present", false)); \
case VK_ERROR_FEATURE_NOT_PRESENT: assert(("[ERROR] Vulkan feature not present", false)); \
case VK_ERROR_INCOMPATIBLE_DRIVER: assert(("[ERROR] Vulkan incompatible driver", false)); \
case VK_ERROR_TOO_MANY_OBJECTS: assert(("[ERROR] Vulkan too many objects", false)); \
case VK_ERROR_FORMAT_NOT_SUPPORTED: assert(("[ERROR] Vulkan format not supported", false)); \
case VK_ERROR_FRAGMENTED_POOL: assert(("[ERROR] Vulkan fragmented pool", false)); \
case VK_ERROR_UNKNOWN: assert(("[ERROR] Vulkan unknown", false)); \
case VK_TIMEOUT: assert(("[ERROR] Vulkan timeout", false)); \
default: assert(("[ERROR] Unknown", false)); \
}
#endif

unsigned int memoryTypeFromProperties(const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, const VkMemoryPropertyFlags& memoryProperties);

int find(const std::vector<char>&string, const std::vector<char>&find, const unsigned int& start = 0);

std::vector<std::vector<char>> split(const std::vector<char>& string, const std::vector<char>& split);

std::vector<char> readFile(const char* directory);

void writeFile(const char* directory, std::vector<char> content);

struct ShaderConstant
{
	const char* name;
	const char* value;
};

void writeConstants(std::vector<char>& string, const std::vector<ShaderConstant>& constants);

template <class T>
std::size_t hash(const T& value)
{
	return std::hash<T>()(value);
}

template <class T>
inline void hashCombine(std::size_t& hash, const T& value)
{
	hash ^= std::hash<T>()(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

template <typename T>
std::vector<char> serialize(const T& data)
{
	const char* byteData = reinterpret_cast<const char*>(&data);
	return std::vector<char>(byteData, byteData + sizeof(T));
}

template <>
std::vector<char> serialize(const std::string& string); // Returns byte data containing the string's length followed by the string

template <typename T>
void deserialize(const std::vector<char>& vecData, T& write)
{
	write = *(reinterpret_cast<const T*>(vecData.data()));
}

template <>
void deserialize(const std::vector<char>& vecData, std::string& write); // Deserializes a string given that vecData has the same length as the string