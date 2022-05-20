#include "FontManager.h"

FontManager::FontManager()
{
    #ifdef _DEBUG
    assert(("[ERROR] FreeType failed to initialize", !FT_Init_FreeType(&mFT)));
    #else
    FT_Init_FreeType(&mFT);
    #endif
}

FontManager& FontManager::instance()
{
	static FontManager instance;
	return instance;
}

FontManager::~FontManager()
{
	RenderSystem& renderSystem = RenderSystem::instance();

	// Free glyphs
	for (const std::pair<std::string, Font*>& font : mFonts)
	{
		for (const std::pair<char, Glyph*>& c : *font.second)
		{
			Glyph* glyph = c.second;
			
			vkFreeDescriptorSets(renderSystem.mDevice, renderSystem.mDescriptorPool, 1, &glyph->descriptorSet);
			vkDestroySampler(renderSystem.mDevice, glyph->sampler, nullptr);
			vkDestroyImageView(renderSystem.mDevice, glyph->imageView, nullptr);
			vkFreeMemory(renderSystem.mDevice, glyph->imageMemory, nullptr);
			vkDestroyImage(renderSystem.mDevice, glyph->image, nullptr);

			delete glyph;
		}
		delete font.second;
	}

	FT_Done_FreeType(mFT);
}

const Font& FontManager::getFont(const char* filename)
{
    std::unordered_map<std::string, Font*>::const_iterator it = mFonts.find(filename);
    if (it != mFonts.end())
        return *it->second;

	// Load font
    FT_Face face;
    #ifdef _DEBUG
    assert(("[ERROR] FreeType failed to load font", !FT_New_Face(mFT, filename, 0, &face)));
    #else
    FT_New_Face(mFT, filename, 0, &face);
    #endif
    FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);

    Font* font = mFonts[filename] = new Font();
    for (unsigned char c = 0; c < 128; c++)
    {
        #ifdef _DEBUG
        assert(("[ERROR] FreeType failed to load a character", !FT_Load_Char(face, c, FT_LOAD_RENDER)));
        #else
        FT_Load_Char(face, c, FT_LOAD_RENDER);
        #endif
		
		Glyph* glyph = font->operator[](c) = new Glyph();
		glyph->size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		glyph->bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
		glyph->advance = face->glyph->advance.x;

		// Load glyph texture; space does not have a texture and hence it's bitmap dimensions equal 0
		if (face->glyph->bitmap.width != 0 && face->glyph->bitmap.rows != 0)
		{
			RenderSystem& renderSystem = RenderSystem::instance();

			size_t imageSize = (size_t)face->glyph->bitmap.width * face->glyph->bitmap.rows;

			// Create image
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.pNext = nullptr;
			imageCreateInfo.flags = 0;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = FORMAT_R;
			imageCreateInfo.extent = { face->glyph->bitmap.width, face->glyph->bitmap.rows, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.queueFamilyIndexCount = 0;
			imageCreateInfo.pQueueFamilyIndices = nullptr;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkResult result = vkCreateImage(renderSystem.mDevice, &imageCreateInfo, nullptr, &glyph->image);
			validateResult(result);

			VkMemoryRequirements memoryRequirements;
			vkGetImageMemoryRequirements(renderSystem.mDevice, glyph->image, &memoryRequirements);

			VkMemoryAllocateInfo memoryAllocateInfo = {};
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(renderSystem.mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			result = vkAllocateMemory(renderSystem.mDevice, &memoryAllocateInfo, nullptr, &glyph->imageMemory);
			validateResult(result);

			result = vkBindImageMemory(renderSystem.mDevice, glyph->image, glyph->imageMemory, 0);
			validateResult(result);

			// Create staging buffer
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.pNext = nullptr;
			bufferCreateInfo.flags = 0;
			bufferCreateInfo.size = imageSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bufferCreateInfo.queueFamilyIndexCount = 0;
			bufferCreateInfo.pQueueFamilyIndices = nullptr;
			result = vkCreateBuffer(renderSystem.mDevice, &bufferCreateInfo, nullptr, &stagingBuffer);
			validateResult(result);

			vkGetBufferMemoryRequirements(renderSystem.mDevice, stagingBuffer, &memoryRequirements);

			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(renderSystem.mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			result = vkAllocateMemory(renderSystem.mDevice, &memoryAllocateInfo, nullptr, &stagingMemory);
			validateResult(result);

			result = vkBindBufferMemory(renderSystem.mDevice, stagingBuffer, stagingMemory, 0);
			validateResult(result);

			// Copy textureData into staging buffer
			unsigned char* data;
			vkMapMemory(renderSystem.mDevice, stagingMemory, 0, imageSize, 0, (void**)&data);
			memcpy(data, face->glyph->bitmap.buffer, imageSize);
			vkUnmapMemory(renderSystem.mDevice, stagingMemory);

			// Copy staging buffer to image, transfer layout
			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBufferBeginInfo.pNext = nullptr;
			commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			commandBufferBeginInfo.pInheritanceInfo = nullptr;
			result = vkBeginCommandBuffer(renderSystem.mCommandBuffer, &commandBufferBeginInfo);
			validateResult(result);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.pNext = nullptr;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = glyph->image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(renderSystem.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageOffset = { 0, 0, 0 };
			copyRegion.imageExtent = { face->glyph->bitmap.width, face->glyph->bitmap.rows, 1 };
			vkCmdCopyBufferToImage(renderSystem.mCommandBuffer, stagingBuffer, glyph->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier(renderSystem.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			result = vkEndCommandBuffer(renderSystem.mCommandBuffer);
			validateResult(result);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.pWaitDstStageMask = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &renderSystem.mCommandBuffer;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;
			result = vkQueueSubmit(renderSystem.mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
			validateResult(result);

			// Create image view
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.pNext = nullptr;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.image = glyph->image;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = FORMAT_R;
			imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO };
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			result = vkCreateImageView(renderSystem.mDevice, &imageViewCreateInfo, nullptr, &glyph->imageView);
			validateResult(result);

			// Create sampler
			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.pNext = nullptr;
			samplerCreateInfo.flags = 0;
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = 1.0f;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
			samplerCreateInfo.maxAnisotropy = renderSystem.mPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
			samplerCreateInfo.compareEnable = VK_FALSE;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
			result = vkCreateSampler(renderSystem.mDevice, &samplerCreateInfo, nullptr, &glyph->sampler);
			validateResult(result);

			// Wait idle until command buffer finishes so it can be reset
			result = vkQueueWaitIdle(renderSystem.mGraphicsQueue);
			validateResult(result);

			// Reset command buffer for next use
			result = vkResetCommandBuffer(renderSystem.mCommandBuffer, 0);
			validateResult(result);

			vkDestroyBuffer(renderSystem.mDevice, stagingBuffer, nullptr);
			vkFreeMemory(renderSystem.mDevice, stagingMemory, nullptr);

			// Create descriptor set
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.pNext = nullptr;
			descriptorSetAllocateInfo.descriptorPool = renderSystem.mDescriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &renderSystem.mImageDescriptorSetLayout;
			vkAllocateDescriptorSets(renderSystem.mDevice, &descriptorSetAllocateInfo, &glyph->descriptorSet);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.sampler = glyph->sampler;
			imageInfo.imageView = glyph->imageView;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet descriptorSetWrite = {};
			descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorSetWrite.pNext = nullptr;
			descriptorSetWrite.dstSet = glyph->descriptorSet;
			descriptorSetWrite.dstBinding = 0;
			descriptorSetWrite.dstArrayElement = 0;
			descriptorSetWrite.descriptorCount = 1;
			descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetWrite.pImageInfo = &imageInfo;
			descriptorSetWrite.pBufferInfo = nullptr;
			descriptorSetWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(renderSystem.mDevice, 1, &descriptorSetWrite, 0, nullptr);
		}
	}
    FT_Done_Face(face);

    return *font;
}
