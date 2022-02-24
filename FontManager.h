#pragma once
#include "Mesh.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include "glm/glm.hpp"

struct Glyph
{
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;

	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory imageMemory = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

typedef std::unordered_map<unsigned char, Glyph*> Font;

class FontManager
{
private:
	FT_Library mFT;

	std::unordered_map<std::string, Font*> mFonts;

	FontManager();

public:
	static FontManager& instance();

	~FontManager();

	const Font& getFont(const char* filename);
};