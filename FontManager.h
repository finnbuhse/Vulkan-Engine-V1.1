#pragma once
#include "Mesh.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include "glm/glm.hpp"

#define FONT_SIZE 48

// A texture of a font character. Should not need to be interacted with
struct Glyph
{
	glm::ivec2 size; // Size in pixels of glyph
	glm::ivec2 bearing; // Offset to correctly position glyph in text
	unsigned int advance; // Number of pixels to advance for the next character

	// Vulkan resources
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
	// Free type instance
	FT_Library mFT;

	// Maps font directories to fonts to prevent loading multiple times
	std::unordered_map<std::string, Font*> mFonts;

	FontManager();

public:
	static FontManager& instance();

	~FontManager();

	/*
	Gets font, if font hasn't been loaded then it will be read from filename, otherwise it is used as a hash key.
	\param filename: The local directory of the font.
	\return A reference to the font.
	*/
	const Font& getFont(const char* filename);
};