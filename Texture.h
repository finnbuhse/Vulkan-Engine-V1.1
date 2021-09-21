#pragma once
#include "Vulkan.h"
#include <unordered_map>
#include <string>

#define RGBA VK_FORMAT_R8G8B8A8_UNORM
#define RGBA_HDR16 VK_FORMAT_R16G16B16A16_SFLOAT
#define RGBA_HDR32 VK_FORMAT_R32G32B32A32_SFLOAT

struct TextureInfo
{
	// Directory of texture relative to the current working directory and format to store pixels loaded
	std::string directory;
	VkFormat format;

	inline bool operator ==(const TextureInfo& right) const
	{
		return directory == right.directory && format == right.format;
	}
};

// Object which includes a hash function used by std::unordered_map to generate a hash based on the texture info
struct TextureInfoHasher
{
	
	inline std::size_t operator()(const TextureInfo& key) const noexcept
	{
		/* Previous hash function
		return std::hash<std::string>()(key.directory) ^ (std::hash<VkFormat>()(key.format) << 1);

		Easier to use, unsure how it compares in performance to previous hash function. See Vulkan.h for implementation */
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
	std::string directories[6]; // ORDER right, left, bottom, top, front, back
	VkFormat format;

	inline bool operator ==(const CubemapInfo& right) const
	{
		// Equal if format and all directiories are equal
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

	// Seperate command buffer to render system, texture manager could perhaps run on seperate thread in the future
	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;

	// Maps to retrieve textures and cubemaps from their info to prevent textures being loaded multiple times
	std::unordered_map<TextureInfo, Texture*, TextureInfoHasher> mTextures;
	std::unordered_map<CubemapInfo, Cubemap*, CubemapInfoHasher> mCubemaps;

	TextureManager();

public:
	// Only one instance of texture manager should exist
	static TextureManager& instance();

	TextureManager(const TextureManager& copy) = delete;
	~TextureManager();

	// Interface with hash maps
	Texture& getTexture(const TextureInfo& textureInfo);
	Cubemap& getCubemap(const CubemapInfo& cubemapInfo);
};