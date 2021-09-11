#include "Texture.h"
#include "Mesh.h"
#include "stb_image.h"
#include <Windows.h>

struct FormatInfo
{
	VkComponentMapping componentMapping;
	unsigned int channels;
	unsigned int bytesPerChannel;
};

FormatInfo getFormatInfo(const VkFormat& format)
{
	FormatInfo formatInfo = {};

	unsigned int uiFormat = (unsigned int)format;
	if (uiFormat > VK_FORMAT_A1R5G5B5_UNORM_PACK16 && uiFormat < VK_FORMAT_R8G8_UNORM)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 1;
		formatInfo.bytesPerChannel = 1;
	}
	else if (uiFormat > VK_FORMAT_R8_SRGB && uiFormat < VK_FORMAT_R8G8B8_UNORM)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 2;
		formatInfo.bytesPerChannel = 1;
	}
	else if (uiFormat > VK_FORMAT_R8G8_SRGB && uiFormat < VK_FORMAT_R8G8B8A8_UNORM)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 3;
		formatInfo.bytesPerChannel = 1;
	}
	else if (uiFormat > VK_FORMAT_B8G8R8_SRGB && uiFormat < VK_FORMAT_A2R10G10B10_UNORM_PACK32)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		formatInfo.channels = 4;
		formatInfo.bytesPerChannel = 1;
	}
	else if (uiFormat > VK_FORMAT_A2B10G10R10_SINT_PACK32 && uiFormat < VK_FORMAT_R16G16_UNORM)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 1;
		formatInfo.bytesPerChannel = 2;
	}
	else if (uiFormat > VK_FORMAT_R16_SFLOAT && uiFormat < VK_FORMAT_R16G16B16_UNORM)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 2;
		formatInfo.bytesPerChannel = 2;
	}
	else if (uiFormat > VK_FORMAT_R16G16_SFLOAT && uiFormat < VK_FORMAT_R16G16B16A16_UNORM)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 3;
		formatInfo.bytesPerChannel = 2;
	}
	else if (uiFormat > VK_FORMAT_R16G16B16_SFLOAT && uiFormat < VK_FORMAT_R32_UINT)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		formatInfo.channels = 4;
		formatInfo.bytesPerChannel = 2;
	}
	else if (uiFormat > VK_FORMAT_R16G16B16A16_SFLOAT && uiFormat < VK_FORMAT_R32G32_UINT)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 1;
		formatInfo.bytesPerChannel = 4;
	}
	else if (uiFormat > VK_FORMAT_R32_SFLOAT && uiFormat < VK_FORMAT_R32G32B32_UINT)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 2;
		formatInfo.bytesPerChannel = 4;
	}
	else if (uiFormat > VK_FORMAT_R32G32_SFLOAT && uiFormat < VK_FORMAT_R32G32B32A32_UINT)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE };
		formatInfo.channels = 3;
		formatInfo.bytesPerChannel = 4;
	}
	else if (uiFormat > VK_FORMAT_R32G32B32_SFLOAT && uiFormat < VK_FORMAT_R64_UINT)
	{
		formatInfo.componentMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		formatInfo.channels = 4;
		formatInfo.bytesPerChannel = 4;
	}
	return formatInfo;
}







#include <iostream>

Texture::Texture(const TextureInfo& textureInfo)
{
	RenderSystem& renderSystem = RenderSystem::instance();
	TextureManager& textureManager = TextureManager::instance();

	VkImageFormatProperties formatProperties;
	assert(("[ERROR] Unsupported texture format", !vkGetPhysicalDeviceImageFormatProperties(renderSystem.mPhysicalDevice, textureInfo.format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, &formatProperties)));
	
	FormatInfo formatInfo = getFormatInfo(textureInfo.format);

	#pragma region Create texture resources
	stbi_set_flip_vertically_on_load(false);

	// Load image
	int width, height, channels;
	unsigned char* textureData = stbi_load(textureInfo.directory.c_str(), &width, &height, &channels, formatInfo.channels);
	assert(("[ERROR] STBI failed to load image", textureData));

	VkDeviceSize imageSize = width * height * formatInfo.channels * formatInfo.bytesPerChannel; // Assuming byte per channel
	unsigned int nMips = unsigned int(std::floor(std::log2(width > height ? width : height))) + 1;

	assert(("[ERROR] Unsupported texture format", formatProperties.maxExtent.width >= width && formatProperties.maxExtent.height >= height && formatProperties.maxExtent.depth >= 1 && formatProperties.maxMipLevels >= 1 && formatProperties.maxArrayLayers >= 1 && formatProperties.sampleCounts & VK_SAMPLE_COUNT_1_BIT && formatProperties.maxResourceSize >= imageSize));

	// Create image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = textureInfo.format;
	imageCreateInfo.extent = { unsigned int(width), unsigned int(height), 1 };
	imageCreateInfo.mipLevels = nMips;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	vkCreateImage(renderSystem.mDevice, &imageCreateInfo, nullptr, &mImage);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(renderSystem.mDevice, mImage, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(renderSystem.mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(renderSystem.mDevice, &memoryAllocateInfo, nullptr, &mImageMemory);

	vkBindImageMemory(renderSystem.mDevice, mImage, mImageMemory, 0);

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
	vkCreateBuffer(renderSystem.mDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

	vkGetBufferMemoryRequirements(renderSystem.mDevice, stagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(renderSystem.mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(renderSystem.mDevice, &memoryAllocateInfo, nullptr, &stagingMemory);

	vkBindBufferMemory(renderSystem.mDevice, stagingBuffer, stagingMemory, 0);

	unsigned char* data;
	vkMapMemory(renderSystem.mDevice, stagingMemory, 0, imageSize, 0, (void**)&data);
	memcpy(data, textureData, imageSize);
	vkUnmapMemory(renderSystem.mDevice, stagingMemory);

	stbi_image_free(textureData);

	// Copy staging buffer to image, generate mipmaps, transfer layout
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(textureManager.mCommandBuffer, &commandBufferBeginInfo);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = mImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = nMips;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(textureManager.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { unsigned int(width), unsigned int(height), 1 };
	vkCmdCopyBufferToImage(textureManager.mCommandBuffer, stagingBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.subresourceRange.levelCount = 1;

	VkImageBlit imageBlit = {};
	imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.srcSubresource.baseArrayLayer = 0;
	imageBlit.srcSubresource.layerCount = 1;
	imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.dstSubresource.baseArrayLayer = 0;
	imageBlit.dstSubresource.layerCount = 1;

	unsigned int mipWidth = width, mipHeight = height;
	for (unsigned int i = 1; i < nMips; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		vkCmdPipelineBarrier(textureManager.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[0] = { 0, 0, 0 };
		imageBlit.srcOffsets[1] = { int(mipWidth), int(mipHeight), 1 };
		
		imageBlit.dstSubresource.mipLevel = i;
		if(mipWidth > 1)
			mipWidth /= 2;
		if(mipHeight > 1)
			mipHeight /= 2;
		imageBlit.dstOffsets[0] = { 0, 0, 0 };
		imageBlit.dstOffsets[1] = { int(mipWidth), int(mipHeight), 1 };

		vkCmdBlitImage(textureManager.mCommandBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
	}

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	for (unsigned int i = 0; i < nMips; i++)
	{
		barrier.oldLayout = i == nMips - 1 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.subresourceRange.baseMipLevel = i;
		vkCmdPipelineBarrier(textureManager.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	vkEndCommandBuffer(textureManager.mCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &textureManager.mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(renderSystem.mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	// Create image view
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = mImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = textureInfo.format;
	imageViewCreateInfo.components = formatInfo.componentMapping;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = nMips;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	vkCreateImageView(renderSystem.mDevice, &imageViewCreateInfo, nullptr, &mImageView);

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
	samplerCreateInfo.maxLod = float(nMips);
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = renderSystem.mPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	vkCreateSampler(renderSystem.mDevice, &samplerCreateInfo, nullptr, &mSampler);

	vkQueueWaitIdle(renderSystem.mGraphicsQueue);
	vkResetCommandBuffer(textureManager.mCommandBuffer, 0);

	vkDestroyBuffer(renderSystem.mDevice, stagingBuffer, nullptr);
	vkFreeMemory(renderSystem.mDevice, stagingMemory, nullptr);
	#pragma endregion
}

Texture::~Texture()
{
	RenderSystem& renderSystem = RenderSystem::instance();
	vkDestroySampler(renderSystem.mDevice, mSampler, nullptr);
	vkDestroyImageView(renderSystem.mDevice, mImageView, nullptr);
	vkFreeMemory(renderSystem.mDevice, mImageMemory, nullptr);
	vkDestroyImage(renderSystem.mDevice, mImage, nullptr);
}

Cubemap::Cubemap(CubemapInfo cubemapInfo)
{
	RenderSystem& renderSystem = RenderSystem::instance();
	TextureManager& textureManager = TextureManager::instance();

	VkImageFormatProperties formatProperties;
	assert(("[ERROR] Unsupported texture format", !vkGetPhysicalDeviceImageFormatProperties(renderSystem.mPhysicalDevice, cubemapInfo.format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, &formatProperties)));

	FormatInfo formatInfo = getFormatInfo(cubemapInfo.format);

    #pragma region Create cubemap resources
	stbi_set_flip_vertically_on_load(true);

	float* hdrTextureData[6];
	unsigned char* textureData[6];
	// Load images
	int width, height, channels;
	bool hdr = cubemapInfo.format == VK_FORMAT_R16_SFLOAT || cubemapInfo.format == VK_FORMAT_R16G16_SFLOAT || cubemapInfo.format == VK_FORMAT_R16G16B16_SFLOAT || cubemapInfo.format == VK_FORMAT_R16G16B16A16_SFLOAT || cubemapInfo.format == VK_FORMAT_R32_SFLOAT || cubemapInfo.format == VK_FORMAT_R32G32_SFLOAT || cubemapInfo.format == VK_FORMAT_R32G32B32_SFLOAT || cubemapInfo.format == VK_FORMAT_R32G32B32A32_SFLOAT;
	if (hdr)
	{
		for (unsigned int i = 0; i < 6; i++)
		{
			hdrTextureData[i] = stbi_loadf(cubemapInfo.directories[i].c_str(), &width, &height, &channels, formatInfo.channels);
		}
	}
	else
	{
		for (unsigned int i = 0; i < 6; i++)
		{
			textureData[i] = stbi_load(cubemapInfo.directories[i].c_str(), &width, &height, &channels, formatInfo.channels);
		}
	}

	const VkDeviceSize imageSize = 6 * VkDeviceSize(width) * height * formatInfo.channels * formatInfo.bytesPerChannel;
	unsigned int nMips = unsigned int(std::floor(std::log2(width > height ? width : height))) + 1;

	assert(("[ERROR] Unsupported texture format", formatProperties.maxExtent.width >= width && formatProperties.maxExtent.height >= height && formatProperties.maxExtent.depth >= 1 && formatProperties.maxMipLevels >= 1 && formatProperties.maxArrayLayers >= 1 && formatProperties.sampleCounts & VK_SAMPLE_COUNT_1_BIT && formatProperties.maxResourceSize >= imageSize));

	// Create image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = cubemapInfo.format;
	imageCreateInfo.extent = { unsigned int(width), unsigned int(height), 1 };
	imageCreateInfo.mipLevels = nMips;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkResult result = vkCreateImage(renderSystem.mDevice, &imageCreateInfo, nullptr, &mImage);
	validateResult(result);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(renderSystem.mDevice, mImage, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(renderSystem.mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	result = vkAllocateMemory(renderSystem.mDevice, &memoryAllocateInfo, nullptr, &mImageMemory);
	validateResult(result);

	result = vkBindImageMemory(renderSystem.mDevice, mImage, mImageMemory, 0);
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

	unsigned char* data;
	result = vkMapMemory(renderSystem.mDevice, stagingMemory, 0, imageSize, 0, (void**)&data);
	validateResult(result);

	unsigned long long dataLayer = unsigned long long(width) * height * formatInfo.channels * formatInfo.bytesPerChannel;
	if (hdr)
	{
		for (unsigned int i = 0; i < 6; i++)
		{
			memcpy((void*)(data + i * dataLayer), (const void*)hdrTextureData[i], dataLayer);
			stbi_image_free((void*)hdrTextureData[i]);
		}
	}
	else
	{
		for (unsigned int i = 0; i < 6; i++)
		{
			memcpy((void*)(data + i * dataLayer), (const void*)textureData[i], dataLayer);
			stbi_image_free((void*)textureData[i]);
		}
	}

	vkUnmapMemory(renderSystem.mDevice, stagingMemory);

	result = vkBeginCommandBuffer(textureManager.mCommandBuffer, &renderSystem.mCommandBufferBeginInfo);
	validateResult(result);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT /* Additional >> */ | VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = mImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = nMips;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 6;
	vkCmdPipelineBarrier(textureManager.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 6;
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { unsigned int(width), unsigned int(height), 1 };
	vkCmdCopyBufferToImage(textureManager.mCommandBuffer, stagingBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.subresourceRange.levelCount = 1;

	VkImageBlit imageBlit = {};
	imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.srcSubresource.baseArrayLayer = 0;
	imageBlit.srcSubresource.layerCount = 6;
	imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBlit.dstSubresource.baseArrayLayer = 0;
	imageBlit.dstSubresource.layerCount = 6;

	unsigned int mipWidth = width, mipHeight = height;
	for (unsigned int i = 1; i < nMips; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		vkCmdPipelineBarrier(textureManager.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[0] = { 0, 0, 0 };
		imageBlit.srcOffsets[1] = { int(mipWidth), int(mipHeight), 1 };

		imageBlit.dstSubresource.mipLevel = i;
		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
		imageBlit.dstOffsets[0] = { 0, 0, 0 };
		imageBlit.dstOffsets[1] = { int(mipWidth), int(mipHeight), 1 };

		vkCmdBlitImage(textureManager.mCommandBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
	}

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	for (unsigned int i = 0; i < nMips; i++)
	{
		barrier.oldLayout = i == nMips - 1 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.subresourceRange.baseMipLevel = i;
		vkCmdPipelineBarrier(textureManager.mCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	result = vkEndCommandBuffer(textureManager.mCommandBuffer);
	validateResult(result);

	VkFence fence;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = 0;
	vkCreateFence(renderSystem.mDevice, &fenceCreateInfo, nullptr, &fence);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &textureManager.mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	result = vkQueueSubmit(renderSystem.mGraphicsQueue, 1, &submitInfo, fence);
	validateResult(result);

	// Create image view
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = mImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCreateInfo.format = cubemapInfo.format;
	imageViewCreateInfo.components = formatInfo.componentMapping;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = nMips;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 6;
	vkCreateImageView(renderSystem.mDevice, &imageViewCreateInfo, nullptr, &mImageView);

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
	samplerCreateInfo.maxLod = float(nMips);
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = renderSystem.mPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	vkCreateSampler(renderSystem.mDevice, &samplerCreateInfo, nullptr, &mSampler);
	
	result = vkQueueWaitIdle(renderSystem.mGraphicsQueue);
	validateResult(result);
	/*
	do
	{
		result = vkWaitForFences(renderSystem.mDevice, 1, &fence, VK_FALSE, 0);
		Sleep(1000);
	} while (result == VK_TIMEOUT);
	*/
	vkResetCommandBuffer(textureManager.mCommandBuffer, 0);

	vkDestroyBuffer(renderSystem.mDevice, stagingBuffer, nullptr);
	vkFreeMemory(renderSystem.mDevice, stagingMemory, nullptr);
	#pragma endregion
}

Cubemap::~Cubemap()
{
	RenderSystem& renderSystem = RenderSystem::instance();
	vkDestroySampler(renderSystem.mDevice, mSampler, nullptr);
	vkDestroyImageView(renderSystem.mDevice, mImageView, nullptr);
	vkFreeMemory(renderSystem.mDevice, mImageMemory, nullptr);
	vkDestroyImage(renderSystem.mDevice, mImage, nullptr);
}

TextureManager::TextureManager()
{
	RenderSystem& renderSystem = RenderSystem::instance();

	vkCreateCommandPool(renderSystem.mDevice, &renderSystem.mGraphicsCommandPoolCreateInfo, nullptr, &mCommandPool);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = mCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(renderSystem.mDevice, &commandBufferAllocateInfo, &mCommandBuffer);
}

TextureManager& TextureManager::instance()
{
	static TextureManager instance;
	return instance;
}

TextureManager::~TextureManager()
{

	for (const std::pair<TextureInfo, Texture*>& pair : mTextures)
		delete pair.second;
	for (const std::pair<CubemapInfo, Cubemap*>& pair : mCubemaps)
		delete pair.second;

	RenderSystem& renderSystem = RenderSystem::instance();
	vkFreeCommandBuffers(renderSystem.mDevice, mCommandPool, 1, &mCommandBuffer);
	vkDestroyCommandPool(renderSystem.mDevice, mCommandPool, nullptr);
}

Texture& TextureManager::getTexture(const TextureInfo& textureInfo)
{
	std::unordered_map<TextureInfo, Texture*>::iterator textureIterator = mTextures.find(textureInfo);
	if (textureIterator != mTextures.end())
		return *textureIterator->second;
	Texture* newTexture = new Texture(textureInfo);
	mTextures.insert({ textureInfo, newTexture });
	return *newTexture;
}

Cubemap& TextureManager::getCubemap(const CubemapInfo& cubemapInfo)
{
	std::unordered_map<CubemapInfo, Cubemap*>::iterator cubemapIterator = mCubemaps.find(cubemapInfo);
	if (cubemapIterator != mCubemaps.end())
		return *cubemapIterator->second;
	Cubemap* newCubemap = new Cubemap(cubemapInfo);
	mCubemaps.insert({ cubemapInfo, newCubemap });
	return *newCubemap;
}
