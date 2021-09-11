#pragma once
#include "Vulkan.h"
#include <unordered_map>
#include <string>

struct TextureInfo
{
	std::string directory;
	VkFormat format;

	inline bool operator ==(const TextureInfo& right) const
	{
		return directory == right.directory && format == right.format;
	}
};

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

struct TextureInfoHasher
{
	
	inline std::size_t operator()(const TextureInfo& key) const noexcept
	{
		/* Previous hash function
		return std::hash<std::string>()(key.directory) ^ (std::hash<VkFormat>()(key.format) << 1); */

		// Easier to use, unsure how it compares performance wise to previous hash function
		std::size_t result = hash(key.directory);
		hashCombine(result, key.format);
		return result;
	};
};

class Texture
{
private:
	friend class TextureManager;
	friend class Mesh;
	friend class RenderSystem;

	Texture(const TextureInfo& textureInfo);

	VkImage mImage = VK_NULL_HANDLE;
	VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
	VkImageView mImageView = VK_NULL_HANDLE;
	VkSampler mSampler = VK_NULL_HANDLE;

public:
	Texture(const Texture& copy) = delete;
	~Texture();
};

struct CubemapInfo
{
	std::string directories[6]; // right, left, bottom, top, front, back
	VkFormat format;

	inline bool operator ==(const CubemapInfo& right) const
	{
		bool equal = format == right.format;
		if (!equal)
			return false;
		for (unsigned int i = 0; i < 6; i++)
		{
			equal &= directories[i] == right.directories[i];
			if (!equal)
				return false;
		}
		return true;
	}
};

struct CubemapInfoHasher
{
	inline std::size_t operator()(const CubemapInfo& key) const noexcept
	{
		std::size_t result = hash(key.directories[0]);
		hashCombine(result, key.directories[1]);
		hashCombine(result, key.directories[2]);
		hashCombine(result, key.directories[3]);
		hashCombine(result, key.directories[4]);
		hashCombine(result, key.directories[5]);
		hashCombine(result, key.format);
		return result;
	};
};

class Cubemap
{
private:
	friend class TextureManager;
	friend class RenderSystem;

	Cubemap(CubemapInfo cubemapInfo);

	VkImage mImage = VK_NULL_HANDLE;
	VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
	VkImageView mImageView = VK_NULL_HANDLE;
	VkSampler mSampler = VK_NULL_HANDLE;

public:
	Cubemap(const Cubemap& copy) = delete;
	~Cubemap();
};

class TextureManager
{
private:
	friend class Texture;
	friend class Cubemap;

	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;

	std::unordered_map<TextureInfo, Texture*, TextureInfoHasher> mTextures;
	std::unordered_map<CubemapInfo, Cubemap*, CubemapInfoHasher> mCubemaps;

	TextureManager();

public:
	static TextureManager& instance();

	TextureManager(const TextureManager& copy) = delete;
	~TextureManager();

	Texture& getTexture(const TextureInfo& textureInfo);
	Cubemap& getCubemap(const CubemapInfo& cubemapInfo);
};