#include "Mesh.h"
#include "FontManager.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/material.h"
#include <stdlib.h>
#include <iostream>

constexpr VkDeviceSize ZERO_OFFSET = 0;

MaterialCreateInfo::operator Material() const
{
	TextureManager& textureManager = TextureManager::instance();

	Material material;
	material.albedo = albedo == "" ? nullptr : &textureManager.getTexture({ albedo, VK_FORMAT_R8G8B8A8_SRGB });
	material.normal = normal == "" ? nullptr : &textureManager.getTexture({ normal, VK_FORMAT_R8G8B8A8_UNORM }); // 3 Channel formats are unsupported
	material.roughness = roughness == "" ? nullptr : &textureManager.getTexture({ roughness, VK_FORMAT_R8_UNORM });
	material.metalness = metalness == "" ? nullptr : &textureManager.getTexture({ metalness, VK_FORMAT_R8_UNORM });
	material.ambientOcclusion = ambientOcclusion == "" ? nullptr : &textureManager.getTexture({ ambientOcclusion, VK_FORMAT_R8_UNORM });
	return material;
}

void Mesh::reallocateBuffers()
{
	static RenderSystem& renderSystem = RenderSystem::instance();

	renderSystem.allocateMeshBuffers(*this);
}

void Mesh::updateBuffers()
{
	static RenderSystem& renderSystem = RenderSystem::instance();

	#pragma region Copy staging buffers to device local buffers
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(renderSystem.mCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = nVertices * sizeof(Vertex);
	vkCmdCopyBuffer(renderSystem.mCommandBuffer, _vertexStagingBuffer, _vertexBuffer, 1, &copyRegion);

	copyRegion.size = nIndices * sizeof(unsigned int);
	vkCmdCopyBuffer(renderSystem.mCommandBuffer, _indexStagingBuffer, _indexBuffer, 1, &copyRegion);

	vkEndCommandBuffer(renderSystem.mCommandBuffer);

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
	vkQueueSubmit(renderSystem.mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(renderSystem.mGraphicsQueue);
	vkResetCommandBuffer(renderSystem.mCommandBuffer, 0);
	#pragma endregion
}

void Mesh::updateMaterial()
{
	RenderSystem& renderSystem = RenderSystem::instance();

	#pragma region Update descriptor set
	VkDescriptorImageInfo albedoInfo = {};
	albedoInfo.sampler = material.albedo->mSampler;
	albedoInfo.imageView = material.albedo->mImageView;
	albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo normalInfo = {};
	normalInfo.sampler = material.normal->mSampler;
	normalInfo.imageView = material.normal->mImageView;
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo roughnessInfo = {};
	roughnessInfo.sampler = material.roughness->mSampler;
	roughnessInfo.imageView = material.roughness->mImageView;
	roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo metalnessInfo = {};
	metalnessInfo.sampler = material.metalness->mSampler;
	metalnessInfo.imageView = material.metalness->mImageView;
	metalnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo ambientOcclusionInfo = {};
	ambientOcclusionInfo.sampler = material.ambientOcclusion->mSampler;
	ambientOcclusionInfo.imageView = material.ambientOcclusion->mImageView;
	ambientOcclusionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorSetWrites[5] = {};
	descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[0].pNext = nullptr;
	descriptorSetWrites[0].dstSet = _descriptorSet;
	descriptorSetWrites[0].dstBinding = 1;
	descriptorSetWrites[0].dstArrayElement = 0;
	descriptorSetWrites[0].descriptorCount = 1;
	descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[0].pImageInfo = &albedoInfo;
	descriptorSetWrites[0].pBufferInfo = nullptr;
	descriptorSetWrites[0].pTexelBufferView = nullptr;

	descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[1].pNext = nullptr;
	descriptorSetWrites[1].dstSet = _descriptorSet;
	descriptorSetWrites[1].dstBinding = 2;
	descriptorSetWrites[1].dstArrayElement = 0;
	descriptorSetWrites[1].descriptorCount = 1;
	descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[1].pImageInfo = &normalInfo;
	descriptorSetWrites[1].pBufferInfo = nullptr;
	descriptorSetWrites[1].pTexelBufferView = nullptr;

	descriptorSetWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[2].pNext = nullptr;
	descriptorSetWrites[2].dstSet = _descriptorSet;
	descriptorSetWrites[2].dstBinding = 3;
	descriptorSetWrites[2].dstArrayElement = 0;
	descriptorSetWrites[2].descriptorCount = 1;
	descriptorSetWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[2].pImageInfo = &roughnessInfo;
	descriptorSetWrites[2].pBufferInfo = nullptr;
	descriptorSetWrites[2].pTexelBufferView = nullptr;

	descriptorSetWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[3].pNext = nullptr;
	descriptorSetWrites[3].dstSet = _descriptorSet;
	descriptorSetWrites[3].dstBinding = 4;
	descriptorSetWrites[3].dstArrayElement = 0;
	descriptorSetWrites[3].descriptorCount = 1;
	descriptorSetWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[3].pImageInfo = &metalnessInfo;
	descriptorSetWrites[3].pBufferInfo = nullptr;
	descriptorSetWrites[3].pTexelBufferView = nullptr;

	descriptorSetWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[4].pNext = nullptr;
	descriptorSetWrites[4].dstSet = _descriptorSet;
	descriptorSetWrites[4].dstBinding = 5;
	descriptorSetWrites[4].dstArrayElement = 0;
	descriptorSetWrites[4].descriptorCount = 1;
	descriptorSetWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[4].pImageInfo = &ambientOcclusionInfo;
	descriptorSetWrites[4].pBufferInfo = nullptr;
	descriptorSetWrites[4].pTexelBufferView = nullptr;
	vkUpdateDescriptorSets(renderSystem.mDevice, 5, descriptorSetWrites, 0, nullptr);
	#pragma endregion
}

std::vector<glm::vec3> Mesh::positions() const
{
	std::vector<glm::vec3> positions;
	for (unsigned int i = 0; i < nVertices; i++)
		positions.push_back(vertices[i].position);
	return positions;
}

template <>
std::vector<char> serialize(const Mesh& mesh)
{
	std::vector<char> result;

	// nVertices
	std::vector<char> vecData = serialize(mesh.nVertices);
	result.insert(result.end(), vecData.begin(), vecData.end());
	
	// Vertices
	for (unsigned int i = 0; i < mesh.nVertices; i++)
	{
		vecData = serialize(mesh.vertices[i]);
		result.insert(result.end(), vecData.begin(), vecData.end());
	}

	// nIndices
	vecData = serialize(mesh.nIndices);
	result.insert(result.end(), vecData.begin(), vecData.end());

	// Indices
	for (unsigned int i = 0; i < mesh.nIndices; i++)
	{
		vecData = serialize(mesh.indices[i]);
		result.insert(result.end(), vecData.begin(), vecData.end());
	}

	// Textures
	vecData = serialize(mesh.material.albedo->mInfo.directory);
	result.insert(result.end(), vecData.begin(), vecData.end());

	vecData = serialize(mesh.material.normal->mInfo.directory);
	result.insert(result.end(), vecData.begin(), vecData.end());

	vecData = serialize(mesh.material.roughness->mInfo.directory);
	result.insert(result.end(), vecData.begin(), vecData.end());

	vecData = serialize(mesh.material.metalness->mInfo.directory);
	result.insert(result.end(), vecData.begin(), vecData.end());

	vecData = serialize(mesh.material.ambientOcclusion->mInfo.directory);
	result.insert(result.end(), vecData.begin(), vecData.end());

	return result;
}

template <>
void deserialize(const std::vector<char>& vecData, Mesh& mesh) // Mesh must be attatched to an entity
{
	// N Vertices, N indices
	const char* p = vecData.data();
	unsigned int begin = 0;
	unsigned int size = sizeof(unsigned int);
	deserialize(std::vector<char>(p, p + size), mesh.nVertices);
	begin += size;
	unsigned int verticesStart = begin;
	unsigned int verticesSize = sizeof(Vertex) * mesh.nVertices;
	begin += verticesSize;

	deserialize(std::vector<char>(p + begin, p + begin + size), mesh.nIndices);
	begin += size;

	mesh.reallocateBuffers();

	// Vertices, Indices
	memcpy(mesh.vertices, p + verticesStart, verticesSize);

	size = sizeof(unsigned int) * mesh.nIndices;
	memcpy(mesh.indices, p + begin, size);
	begin += size;

	mesh.updateBuffers();

	// Albedo
	size = sizeof(unsigned int);
	unsigned int stringSize;
	deserialize(std::vector<char>(p + begin, p + begin + size), stringSize);
	begin += size;

	size = stringSize;
	std::string textureString;
	deserialize(std::vector<char>(p + begin, p + begin + size), textureString);
	mesh.material.albedo = &TextureManager::instance().getTexture({ textureString, FORMAT_RGBA_SRGB });
	begin += size;

	// Normal
	size = sizeof(unsigned int);
	deserialize(std::vector<char>(p + begin, p + begin + size), stringSize);
	begin += size;

	size = stringSize;
	deserialize(std::vector<char>(p + begin, p + begin + size), textureString);
	mesh.material.normal = &TextureManager::instance().getTexture({ textureString, FORMAT_RGBA });
	begin += size;

	// Roughness
	size = sizeof(unsigned int);
	deserialize(std::vector<char>(p + begin, p + begin + size), stringSize);
	begin += size;

	size = stringSize;
	deserialize(std::vector<char>(p + begin, p + begin + size), textureString);
	mesh.material.roughness = &TextureManager::instance().getTexture({ textureString, FORMAT_R });
	begin += size;

	// Metalness
	size = sizeof(unsigned int);
	deserialize(std::vector<char>(p + begin, p + begin + size), stringSize);
	begin += size;

	size = stringSize;
	deserialize(std::vector<char>(p + begin, p + begin + size), textureString);
	mesh.material.metalness = &TextureManager::instance().getTexture({ textureString, FORMAT_R });
	begin += size;

	// Ambient occlusion
	size = sizeof(unsigned int);
	deserialize(std::vector<char>(p + begin, p + begin + size), stringSize);
	begin += size;

	size = stringSize;
	deserialize(std::vector<char>(p + begin, p + begin + size), textureString);
	mesh.material.ambientOcclusion = &TextureManager::instance().getTexture({ textureString, FORMAT_R });

	mesh.updateMaterial();
}

DirectionalLightCreateInfo::operator DirectionalLight() const
{
	DirectionalLight light;
	light._lastColour = colour;
	light.colour = colour;
	return light;
}

SpriteCreateInfo::operator Sprite()
{
	TextureManager& textureManager = TextureManager::instance();

	Sprite sprite;
	sprite.width = width;
	sprite.height = height;
	sprite.texture = &textureManager.getTexture({ texture, FORMAT_RGBA });

	return sprite;
}

UIButtonCreateInfo::operator UIButton() const
{
	TextureManager& textureManager = TextureManager::instance();

	UIButton uiButton;
	uiButton.width = width;
	uiButton.height = height;
	uiButton.colour = colour;
	uiButton.unpressedTexture = &textureManager.getTexture({ unpressed, FORMAT_RGBA });
	uiButton.canpressTexture = &textureManager.getTexture({ canpress, FORMAT_RGBA });
	uiButton.pressedTexture = &textureManager.getTexture({ pressed, FORMAT_RGBA });
	uiButton.callback = callback;
	uiButton.toggle = toggle;
	uiButton._pressed = false;
	uiButton._underCursor = false;

	return uiButton;
}

RenderSystem::RenderSystem()
{
	// Subcribe to managers to recieve events
	mTransformManager.subscribeAddedEvent(&mTransformAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mTransformRemovedCallback);
	mMeshManager.subscribeAddedEvent(&mMeshAddedCallback);
	mMeshManager.subscribeRemovedEvent(&mMeshRemovedCallback);
	mDirectionalLightManager.subscribeAddedEvent(&mDirectionalLightAddedCallback);
	mDirectionalLightManager.subscribeRemovedEvent(&mDirectionalLightRemovedCallback);
	mTransform2DManager.subscribeAddedEvent(&mTransform2DAddedCallback);
	mTransform2DManager.subscribeRemovedEvent(&mTransform2DRemovedCallback);
	mSpriteManager.subscribeAddedEvent(&mSpriteAddedCallback);
	mSpriteManager.subscribeRemovedEvent(&mSpriteRemovedCallback);
	mUITextManager.subscribeAddedEvent(&mUITextAddedCallback);
	mUITextManager.subscribeRemovedEvent(&mUITextRemovedCallback);
	mUIButtonManager.subscribeAddedEvent(&mUIButtonAddedCallback);
	mUIButtonManager.subscribeRemovedEvent(&mUIButtonRemovedCallback);
	mWindowManager.subscribeLMBPressedEvent(&mLMBPressedCallback);

	// Compositions define criteria for entities to be included in the system
	mMeshComposition = mTransformManager.bit | mMeshManager.bit;
	mDirectionalLightComposition = mTransformManager.bit | mDirectionalLightManager.bit;

	mSpriteComposition = mTransform2DManager.bit | mSpriteManager.bit;
	mUITextComposition = mTransform2DManager.bit | mUITextManager.bit;
	mUIButtonComposition = mTransform2DManager.bit | mUIButtonManager.bit;

	// Write constants to shader before compiling
	// Demonstrates how shaders could be configured and compiled at runtime
	std::vector<char> fragmentSource = readFile("ShaderSource/shader.frag");
	writeConstants(fragmentSource, { {"MAX_PREFILTER_LOD", std::to_string(mNPrefilterMips).c_str() }, {"MAX_DIRECTIONAL_LIGHTS", std::to_string(MAX_DIRECTIONAL_LIGHTS).c_str() }, {"MAX_POINT_LIGHTS", std::to_string(MAX_POINT_LIGHTS).c_str() }, {"MAX_SPOT_LIGHTS", std::to_string(MAX_SPOT_LIGHTS).c_str() } });
	writeFile("ShaderSource/shader.frag", fragmentSource);

	// Compile shaders so user doesn't have to manually compile after each change
	std::cout << "Compiling shaders..." << std::endl;
	system("glslangValidator.exe -V ShaderSource/shader.vert -o vertex.spv");
	system("glslangValidator.exe -V ShaderSource/shader.frag -o fragment.spv");
	system("glslangValidator.exe -V ShaderSource/cubemap.vert -o cubemapVertex.spv");
	system("glslangValidator.exe -V ShaderSource/cubemap.geom -o cubemapGeometry.spv");
	system("glslangValidator.exe -V ShaderSource/convolute.frag -o convoluteFragment.spv");
	system("glslangValidator.exe -V ShaderSource/prefilter.frag -o prefilterFragment.spv");
	system("glslangValidator.exe -V ShaderSource/skybox.vert -o skyboxVertex.spv");
	system("glslangValidator.exe -V ShaderSource/skybox.frag -o skyboxFragment.spv");
	system("glslangValidator.exe -V ShaderSource/2DQuad.vert -o 2DQuadVertex.spv");
	system("glslangValidator.exe -V ShaderSource/ui.frag -o uiFragment.spv");
	std::cout << "Finished." << std::endl;

	/* VULKAN CONFIGURATION */
	const char* layers[1] = { "VK_LAYER_KHRONOS_validation" };

	// *INSTANCE EXTENSIONS ARE NOT CURRENTLY CONFIGURABLE, ONLY REQUIRED ARE USED*

	const char* deviceExtensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	/* -------------------- */

	#pragma region Create vulkan instance
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = mWindowManager.mWindowTitle;
	applicationInfo.applicationVersion = 1;
	applicationInfo.pEngineName = mWindowManager.mWindowTitle;
	applicationInfo.engineVersion = 1;
	applicationInfo.apiVersion = VK_API_VERSION_1_2;

	// Check chosen validation layers are supported
	if (layers)
	{
		unsigned int nAvailableLayers = 0;
		vkEnumerateInstanceLayerProperties(&nAvailableLayers, nullptr);
		VkLayerProperties* availableLayers = new VkLayerProperties[nAvailableLayers];
		vkEnumerateInstanceLayerProperties(&nAvailableLayers, availableLayers);
		for (unsigned int i = 0; i < sizeof(layers) / sizeof(const char*); i++)
		{
			bool found = false;
			for (unsigned int j = 0; j < nAvailableLayers; j++)
			{
				if (strcmp(layers[i], availableLayers[j].layerName) == 0)
				{
					found = true;
					break;
				}
			}
			assert((found, "[ERROR] Validation layer unsupported"));
		}
		delete[] availableLayers;
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = layers == nullptr ? 0 : sizeof(layers)/sizeof(unsigned char*);
	instanceCreateInfo.ppEnabledLayerNames = layers;
	instanceCreateInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&instanceCreateInfo.enabledExtensionCount); // Use required instance extensions
	vkCreateInstance(&instanceCreateInfo, nullptr, &mVkInstance);
	#pragma endregion

	glfwCreateWindowSurface(mVkInstance, mWindowManager.mWindow, nullptr, &mSurface);

	#pragma region Create device
	vkEnumeratePhysicalDevices(mVkInstance, &mNPhysicalDevices, nullptr);
	assert(("[ERROR] No physical device found", mNPhysicalDevices));
	mPhysicalDevices = new VkPhysicalDevice[mNPhysicalDevices];
	vkEnumeratePhysicalDevices(mVkInstance, &mNPhysicalDevices, mPhysicalDevices);

	/* Selects physical device; if multiple GPUs are present the first discrete GPU found is selected.
	   Otherwise the one GPU is selected */
	if (mNPhysicalDevices > 1)
	{
		for (unsigned int i = 0; i < mNPhysicalDevices; i++)
		{
			vkGetPhysicalDeviceProperties(mPhysicalDevices[i], &mPhysicalDeviceProperties);
			if (mPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				mPhysicalDevice = mPhysicalDevices[i];
				break;
			}
		}
	}
	else
	{
		mPhysicalDevice = mPhysicalDevices[0];
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &mPhysicalDeviceProperties);
	}
	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mPhysicalDeviceMemoryProperties);

	/* Attempt to find queue family that suppports both graphics and presenting.
	Otherwise use seperate queue families */
	unsigned int nQueueFamilies;
	vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &nQueueFamilies, nullptr);
	VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[nQueueFamilies];
	vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &nQueueFamilies, queueFamilyProperties);

	bool* queueFamilyPresentSupport = new bool[nQueueFamilies];
	for (unsigned int i = 0; i < nQueueFamilies; i++)
		vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurface, (VkBool32*)&queueFamilyPresentSupport[i]);

	for (unsigned int i = 0; i < nQueueFamilies; i++)
	{
		if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			mGraphicsQueueIndex = i;
			if (queueFamilyPresentSupport[i])
			{
				mPresentQueueIndex = i;
				break;
			}
		}
		else if (queueFamilyPresentSupport[i])
			mPresentQueueIndex = i;
	}
	delete[] queueFamilyProperties, queueFamilyPresentSupport;
	assert(("[ERROR] None of the physical device's queue families support graphics", mGraphicsQueueIndex != UINT32_MAX));
	assert(("[ERROR] None of the physical device's queue families support presenting", mPresentQueueIndex != UINT32_MAX));

	// Check device features are supported
	VkBool32* requiredFeatures = reinterpret_cast<VkBool32*>(&deviceFeatures);
	vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mPhysicalDeviceFeatures);
	VkBool32* availableFeatures = reinterpret_cast<VkBool32*>(&mPhysicalDeviceFeatures);
	for (unsigned int i = 0; i < N_VK_DEVICE_FEATURES; i++)
	{
		if(requiredFeatures[i])
			assert(("[ERROR] Physical device missing support for required feature", availableFeatures[i]));
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	float queuePriority = 0.0f;
	if (mGraphicsQueueIndex == mPresentQueueIndex)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = mGraphicsQueueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
	}
	else
	{
		VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
		queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[0].pNext = nullptr;
		queueCreateInfos[0].flags = 0;
		queueCreateInfos[0].queueFamilyIndex = mGraphicsQueueIndex;
		queueCreateInfos[0].queueCount = 1;
		queueCreateInfos[0].pQueuePriorities = &queuePriority;

		queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[1].pNext = nullptr;
		queueCreateInfos[1].flags = 0;
		queueCreateInfos[1].queueFamilyIndex = mPresentQueueIndex;
		queueCreateInfos[1].queueCount = 1;
		queueCreateInfos[1].pQueuePriorities = &queuePriority;

		deviceCreateInfo.queueCreateInfoCount = 2;
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
		vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
	}

	vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, mPresentQueueIndex, 0, &mPresentQueue);
	#pragma endregion

	#pragma region Create swapchain
	// Check presenting image format is supported
	unsigned int nAvailableFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &nAvailableFormats, nullptr);
	VkSurfaceFormatKHR* availableFormats = new VkSurfaceFormatKHR[nAvailableFormats];
	vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &nAvailableFormats, availableFormats);

	bool supported = nAvailableFormats == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED;
	if (!supported)
	{
		for (unsigned int i = 0; i < nAvailableFormats; i++)
		{
			if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				supported = true;
				break;
			}
		}
	}
	delete[] availableFormats;
	assert(("[ERROR] Surface format VK_FORMAT_B8G8R8A8_UNORM unsupported", supported));

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &surfaceCapabilities);

	if (surfaceCapabilities.currentExtent.width == UINT32_MAX)
	{
		if (mWindowManager.mWindowWidth < surfaceCapabilities.minImageExtent.width)
			mSurfaceWidth = surfaceCapabilities.minImageExtent.width;
		else if (mWindowManager.mWindowWidth > surfaceCapabilities.maxImageExtent.width)
			mSurfaceWidth = surfaceCapabilities.maxImageExtent.width;
		else
			mSurfaceWidth = mWindowManager.mWindowWidth;

		if (mWindowManager.mWindowHeight < surfaceCapabilities.minImageExtent.height)
			mSurfaceHeight = surfaceCapabilities.minImageExtent.height;
		else if (mWindowManager.mWindowHeight > surfaceCapabilities.maxImageExtent.height)
			mSurfaceHeight = surfaceCapabilities.maxImageExtent.height;
		else
			mSurfaceHeight = mWindowManager.mWindowHeight;
	}
	else
	{
		mSurfaceWidth = surfaceCapabilities.currentExtent.width;
		mSurfaceHeight = surfaceCapabilities.currentExtent.height;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = mSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageExtent = { mSurfaceWidth, mSurfaceHeight };
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// preTransform
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;

	// compositeAlpha
	VkCompositeAlphaFlagBitsKHR compositeAlphas[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR };

	for (unsigned int i = 0; i < sizeof(compositeAlphas) / sizeof(VkCompositeAlphaFlagBitsKHR); i++)
	{
		if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphas[i])
		{
			swapchainCreateInfo.compositeAlpha = compositeAlphas[i];
			break;
		}
	}

    #if VSYNC == true
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	#else
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	#endif
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// imageSharingMode, queueFamilyIndexCount, pQueueFamilyIndices 
	if (mGraphicsQueueIndex == mPresentQueueIndex)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		vkCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain);
	}
	else
	{
		unsigned int queueFamilyIndices[2] = { mGraphicsQueueIndex, mPresentQueueIndex };

		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		vkCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain);
	}

	vkGetSwapchainImagesKHR(mDevice, mSwapchain, &mNSwapchainImages, nullptr);
	VkImage* swapchainImages = new VkImage[mNSwapchainImages];
	vkGetSwapchainImagesKHR(mDevice, mSwapchain, &mNSwapchainImages, swapchainImages);

	// Create swapchain image views
	mSwapchainImageViews = new VkImageView[mNSwapchainImages];

	VkImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	
	for (unsigned int i = 0; i < mNSwapchainImages; i++)
	{
		imageViewCreateInfo.image = swapchainImages[i];
		vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mSwapchainImageViews[i]);
	}

	delete[] swapchainImages;
	#pragma endregion

	#pragma region Create depth buffer
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_D16_UNORM;
	imageCreateInfo.extent = { mSurfaceWidth, mSurfaceHeight, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(mDevice, &imageCreateInfo, nullptr, &mDepthImage);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(mDevice, mDepthImage, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mDepthMemory);

	vkBindImageMemory(mDevice, mDepthImage, mDepthMemory, 0);

	imageViewCreateInfo.image = mDepthImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = VK_FORMAT_D16_UNORM;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mDepthImageView);
	#pragma endregion

	#pragma region Create irradiance cubemap
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageCreateInfo.extent = { IRRADIANCE_WIDTH_HEIGHT, IRRADIANCE_WIDTH_HEIGHT, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(mDevice, &imageCreateInfo, nullptr, &mIrradianceImage);

	vkGetImageMemoryRequirements(mDevice, mIrradianceImage, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mIrradianceMemory);

	vkBindImageMemory(mDevice, mIrradianceImage, mIrradianceMemory, 0);

	// Create image view
	imageViewCreateInfo.image = mIrradianceImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT; // 3 Channel formats are unsupported by my GPU
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 6;
	vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mIrradianceImageView);

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
	samplerCreateInfo.maxLod = 0.0f;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = mPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	vkCreateSampler(mDevice, &samplerCreateInfo, nullptr, &mIrradianceSampler);
	#pragma endregion

	#pragma region Create prefilter cubemap
	mNPrefilterMips = (unsigned int)std::floor(std::log2(PREFILTER_WIDTH_HEIGHT)) + 1;

	// Create image
	imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageCreateInfo.extent = { (unsigned int)PREFILTER_WIDTH_HEIGHT, (unsigned int)PREFILTER_WIDTH_HEIGHT, 1 };
	imageCreateInfo.mipLevels = mNPrefilterMips;
	imageCreateInfo.arrayLayers = 6;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateImage(mDevice, &imageCreateInfo, nullptr, &mPrefilterImage);

	vkGetImageMemoryRequirements(mDevice, mPrefilterImage, &memoryRequirements);

	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mPrefilterMemory);

	vkBindImageMemory(mDevice, mPrefilterImage, mPrefilterMemory, 0);

	// Create image view for each mip level
	mPrefilterImageViews = new VkImageView[mNPrefilterMips];
	for (unsigned int i = 0; i < mNPrefilterMips; i++)
	{
		imageViewCreateInfo.image = mPrefilterImage;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = i;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 6;
		vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mPrefilterImageViews[i]);
	}

	// Create image view for all mip levels
	imageViewCreateInfo.image = mPrefilterImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = mNPrefilterMips;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 6;
	vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mPrefilterImageView);

	// Create sampler
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = (float)mNPrefilterMips;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = mPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	vkCreateSampler(mDevice, &samplerCreateInfo, nullptr, &mPrefilterSampler);
	#pragma endregion

	#pragma region Create render passes
	// Create main render pass
	VkAttachmentDescription attachmentDescriptions[2] = {};
	attachmentDescriptions[0].flags = 0;
	attachmentDescriptions[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDescriptions[1].flags = 0;
	attachmentDescriptions[1].format = VK_FORMAT_D16_UNORM;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescriptions[3] = {};
	subpassDescriptions[0].flags = 0;
	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].inputAttachmentCount = 0;
	subpassDescriptions[0].pInputAttachments = nullptr;
	subpassDescriptions[0].colorAttachmentCount = 1;
	subpassDescriptions[0].pColorAttachments = &colorReference;
	subpassDescriptions[0].pResolveAttachments = nullptr;
	subpassDescriptions[0].pDepthStencilAttachment = &depthReference;
	subpassDescriptions[0].preserveAttachmentCount = 0;
	subpassDescriptions[0].pPreserveAttachments = nullptr;
	subpassDescriptions[2] = subpassDescriptions[1] = subpassDescriptions[0];

	VkSubpassDependency subpassDependencies[3] = {};
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = 0;
	subpassDependencies[0].dstAccessMask = 0;
	subpassDependencies[0].dependencyFlags = 0;

	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = 1;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = 0;
	subpassDependencies[1].dstAccessMask = 0;
	subpassDependencies[1].dependencyFlags = 0;

	subpassDependencies[2].srcSubpass = 1;
	subpassDependencies[2].dstSubpass = 2;
	subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[2].srcAccessMask = 0;
	subpassDependencies[2].dstAccessMask = 0;
	subpassDependencies[2].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 3;
	renderPassCreateInfo.pSubpasses = subpassDescriptions;
	renderPassCreateInfo.dependencyCount = 3;
	renderPassCreateInfo.pDependencies = subpassDependencies;
	vkCreateRenderPass(mDevice, &renderPassCreateInfo, nullptr, &mMainRenderPass);

	mClearColours[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	mClearColours[1].depthStencil = { 1.0f, 0 };

	mRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	mRenderPassBeginInfo.pNext = nullptr;
	mRenderPassBeginInfo.renderPass = mMainRenderPass;
	mRenderPassBeginInfo.renderArea = { { 0, 0 }, { mSurfaceWidth, mSurfaceHeight } };
	mRenderPassBeginInfo.clearValueCount = 2;
	mRenderPassBeginInfo.pClearValues = mClearColours;

	// Create environment render pass
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.flags = 0;
	attachmentDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;
	vkCreateRenderPass(mDevice, &renderPassCreateInfo, nullptr, &mEnvironmentRenderPass);

	mEnvironmentClearColour.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	mConvoluteRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	mConvoluteRenderPassBeginInfo.pNext = nullptr;
	mConvoluteRenderPassBeginInfo.renderPass = mEnvironmentRenderPass;
	mConvoluteRenderPassBeginInfo.renderArea = { { 0, 0 }, { IRRADIANCE_WIDTH_HEIGHT, IRRADIANCE_WIDTH_HEIGHT } };
	mConvoluteRenderPassBeginInfo.clearValueCount = 1;
	mConvoluteRenderPassBeginInfo.pClearValues = &mEnvironmentClearColour;
	#pragma endregion

	#pragma region Create framebuffers
	// Create main framebuffers
	VkImageView attachments[2] = {};
	attachments[1] = mDepthImageView;

	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = nullptr;
	framebufferCreateInfo.flags = 0;
	framebufferCreateInfo.renderPass = mMainRenderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = attachments;
	framebufferCreateInfo.width = mSurfaceWidth;
	framebufferCreateInfo.height = mSurfaceHeight;
	framebufferCreateInfo.layers = 1;

	mFramebuffers = new VkFramebuffer[mNSwapchainImages];
	for (unsigned int i = 0; i < mNSwapchainImages; i++)
	{
		attachments[0] = mSwapchainImageViews[i];
		vkCreateFramebuffer(mDevice, &framebufferCreateInfo, nullptr, &mFramebuffers[i]);
	}

	// Create convolution framebuffer
	framebufferCreateInfo.renderPass = mEnvironmentRenderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = &mIrradianceImageView;
	framebufferCreateInfo.width = IRRADIANCE_WIDTH_HEIGHT;
	framebufferCreateInfo.height = IRRADIANCE_WIDTH_HEIGHT;
	framebufferCreateInfo.layers = 6;
	vkCreateFramebuffer(mDevice, &framebufferCreateInfo, nullptr, &mConvoluteFramebuffer);

	mConvoluteRenderPassBeginInfo.framebuffer = mConvoluteFramebuffer;

	// Create prefilter framebuffers
	framebufferCreateInfo.renderPass = mEnvironmentRenderPass;
	framebufferCreateInfo.layers = 6;
	framebufferCreateInfo.attachmentCount = 1;
	mPrefilterFramebuffers = new VkFramebuffer[mNPrefilterMips];
	for (unsigned int i = 0; i < mNPrefilterMips; i++)
	{
		framebufferCreateInfo.pAttachments = &mPrefilterImageViews[i];
		framebufferCreateInfo.width = framebufferCreateInfo.height = (float)PREFILTER_WIDTH_HEIGHT * std::pow(0.5, i);
		vkCreateFramebuffer(mDevice, &framebufferCreateInfo, nullptr, &mPrefilterFramebuffers[i]);
	}
	#pragma endregion

	#pragma region Create uniform buffer
	// Stores camera's projection and view matrices aswell as its position
	unsigned int bufferRange = 2 * sizeof(glm::mat4) + sizeof(glm::vec3);
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = bufferRange;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mCameraUniformBuffer);

	vkGetBufferMemoryRequirements(mDevice, mCameraUniformBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mCameraUniformMemory);

	vkBindBufferMemory(mDevice, mCameraUniformBuffer, mCameraUniformMemory, 0);

	vkMapMemory(mDevice, mCameraUniformMemory, 0, bufferRange, 0, (void**)&mUniformData);
	#pragma endregion

	#pragma region Create descriptor pool and descriptor layouts
	// Create descriptor pool
	VkDescriptorPoolSize descriptorPoolSizes[2] = {};
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[0].descriptorCount = 200;

	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorPoolSizes[1].descriptorCount = 400;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolCreateInfo.maxSets = 500;
	descriptorPoolCreateInfo.poolSizeCount = 2;
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
	vkCreateDescriptorPool(mDevice, &descriptorPoolCreateInfo, nullptr, &mDescriptorPool);

	// Create layout 0 in 'mPipeline'
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings0[4] = {};
	descriptorSetLayoutBindings0[0].binding = 0;
	descriptorSetLayoutBindings0[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings0[0].descriptorCount = 1;
	descriptorSetLayoutBindings0[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings0[0].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings0[1].binding = 1;
	descriptorSetLayoutBindings0[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings0[1].descriptorCount = 1;
	descriptorSetLayoutBindings0[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings0[1].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings0[2].binding = 2;
	descriptorSetLayoutBindings0[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings0[2].descriptorCount = 1;
	descriptorSetLayoutBindings0[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings0[2].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings0[3].binding = 3;
	descriptorSetLayoutBindings0[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings0[3].descriptorCount = 1;
	descriptorSetLayoutBindings0[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings0[3].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = nullptr;
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.bindingCount = 4;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings0;
	vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &mDirectionalLightingDescriptorSetLayouts[0]);

	// Create layout 1 in 'mPipeline'
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings1[6] = {};
	descriptorSetLayoutBindings1[0].binding = 0;
	descriptorSetLayoutBindings1[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings1[0].descriptorCount = 1;
	descriptorSetLayoutBindings1[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings1[0].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings1[1].binding = 1;
	descriptorSetLayoutBindings1[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings1[1].descriptorCount = 1;
	descriptorSetLayoutBindings1[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings1[1].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings1[2].binding = 2;
	descriptorSetLayoutBindings1[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings1[2].descriptorCount = 1;
	descriptorSetLayoutBindings1[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings1[2].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings1[3].binding = 3;
	descriptorSetLayoutBindings1[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings1[3].descriptorCount = 1;
	descriptorSetLayoutBindings1[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings1[3].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings1[4].binding = 4;
	descriptorSetLayoutBindings1[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings1[4].descriptorCount = 1;
	descriptorSetLayoutBindings1[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings1[4].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings1[5].binding = 5;
	descriptorSetLayoutBindings1[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings1[5].descriptorCount = 1;
	descriptorSetLayoutBindings1[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings1[5].pImmutableSamplers = nullptr;

	descriptorSetLayoutCreateInfo.bindingCount = 6;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings1;
	vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &mDirectionalLightingDescriptorSetLayouts[1]);

	// Create layout 2 in 'mPipeline'
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding = 0;
	descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
	vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &mDirectionalLightingDescriptorSetLayouts[2]);

	// Create layout 0 in 'mSkyboxPipeline'
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings2[2] = {};
	descriptorSetLayoutBindings2[0].binding = 0;
	descriptorSetLayoutBindings2[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings2[0].descriptorCount = 1;
	descriptorSetLayoutBindings2[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings2[0].pImmutableSamplers = nullptr;
	   
	descriptorSetLayoutBindings2[1].binding = 1;
	descriptorSetLayoutBindings2[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings2[1].descriptorCount = 1;
	descriptorSetLayoutBindings2[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings2[1].pImmutableSamplers = nullptr;
	
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings2;
	vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &mSkyboxDescriptorSetLayout);

	// Create layout 0 in 'mConvolutePipeline' and layout 0 in 'mPrefilterPipeline'
	descriptorSetLayoutBinding.binding = 0;
	descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
	vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &mEnvironmentDescriptorSetLayout);

	// Create layout 0 in 'mUITextPipeline'
	descriptorSetLayoutBinding.binding = 0;
	descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
	vkCreateDescriptorSetLayout(mDevice, &descriptorSetLayoutCreateInfo, nullptr, &mImageDescriptorSetLayout);

	// Allocate descriptor sets
	VkDescriptorSetLayout descriptorSetLayouts[3] = { mDirectionalLightingDescriptorSetLayouts[0], mSkyboxDescriptorSetLayout, mEnvironmentDescriptorSetLayout};
	VkDescriptorSet descriptorSets[3] = {};

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 3;
	descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts;
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, descriptorSets);
	mCameraDescriptorSet = descriptorSets[0];
	mSkyboxDescriptorSet = descriptorSets[1];
	mEnvironmentDescriptorSet = descriptorSets[2];

	VkDescriptorBufferInfo uniformBufferInfo = {};
	uniformBufferInfo.buffer = mCameraUniformBuffer;
	uniformBufferInfo.offset = 0;
	uniformBufferInfo.range = bufferRange;

	VkDescriptorImageInfo irradianceImageInfo = {};
	irradianceImageInfo.sampler = mIrradianceSampler;
	irradianceImageInfo.imageView = mIrradianceImageView;
	irradianceImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo prefilterImageInfo = {};
	prefilterImageInfo.sampler = mPrefilterSampler;
	prefilterImageInfo.imageView = mPrefilterImageView;
	prefilterImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorSetWrites[4] = {};
	descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[0].pNext = nullptr;
	descriptorSetWrites[0].dstSet = mCameraDescriptorSet;
	descriptorSetWrites[0].dstBinding = 0;
	descriptorSetWrites[0].dstArrayElement = 0;
	descriptorSetWrites[0].descriptorCount = 1;
	descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetWrites[0].pImageInfo = nullptr;
	descriptorSetWrites[0].pBufferInfo = &uniformBufferInfo;
	descriptorSetWrites[0].pTexelBufferView = nullptr;

	descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[1].pNext = nullptr;
	descriptorSetWrites[1].dstSet = mCameraDescriptorSet;
	descriptorSetWrites[1].dstBinding = 1;
	descriptorSetWrites[1].dstArrayElement = 0;
	descriptorSetWrites[1].descriptorCount = 1;
	descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[1].pImageInfo = &irradianceImageInfo;
	descriptorSetWrites[1].pBufferInfo = nullptr;
	descriptorSetWrites[1].pTexelBufferView = nullptr;

	descriptorSetWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[2].pNext = nullptr;
	descriptorSetWrites[2].dstSet = mCameraDescriptorSet;
	descriptorSetWrites[2].dstBinding = 2;
	descriptorSetWrites[2].dstArrayElement = 0;
	descriptorSetWrites[2].descriptorCount = 1;
	descriptorSetWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[2].pImageInfo = &prefilterImageInfo;
	descriptorSetWrites[2].pBufferInfo = nullptr;
	descriptorSetWrites[2].pTexelBufferView = nullptr;

	descriptorSetWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[3].pNext = nullptr;
	descriptorSetWrites[3].dstSet = mSkyboxDescriptorSet;
	descriptorSetWrites[3].dstBinding = 0;
	descriptorSetWrites[3].dstArrayElement = 0;
	descriptorSetWrites[3].descriptorCount = 1;
	descriptorSetWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetWrites[3].pImageInfo = nullptr;
	descriptorSetWrites[3].pBufferInfo = &uniformBufferInfo;
	descriptorSetWrites[3].pTexelBufferView = nullptr;
	
	vkUpdateDescriptorSets(mDevice, 4, descriptorSetWrites, 0, nullptr);
	#pragma endregion

	#pragma region Create shaders
	std::vector<char> shaderCode = readFile("vertex.spv");

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mVertexShader);

	shaderCode = readFile("fragment.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mFragmentShader);

	shaderCode = readFile("skyboxVertex.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mSkyboxVertexShader);

	shaderCode = readFile("skyboxFragment.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mSkyboxFragmentShader);

	shaderCode = readFile("cubemapVertex.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mCubemapVertexShader);

	shaderCode = readFile("cubemapGeometry.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mCubemapGeometryShader);

	shaderCode = readFile("convoluteFragment.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mConvoluteFragmentShader);

	shaderCode = readFile("prefilterFragment.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mPrefilterFragmentShader);

	shaderCode = readFile("2DQuadVertex.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &mQuadVertexShader);

	shaderCode = readFile("uiFragment.spv");
	shaderModuleCreateInfo.codeSize = shaderCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<unsigned int*>(shaderCode.data());
	vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &m2DFragmentShader);
	#pragma endregion

	#pragma region Create pipelines
	// Create PBR pipeline
	VkPipelineShaderStageCreateInfo shaderStages[2] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].pNext = nullptr;
	shaderStages[0].flags = 0;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = mVertexShader;
	shaderStages[0].pName = "main";
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].pNext = nullptr;
	shaderStages[1].flags = 0;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = mFragmentShader;
	shaderStages[1].pName = "main";
	shaderStages[1].pSpecializationInfo = nullptr;

	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(Vertex);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Defines format of vertex data
	VkVertexInputAttributeDescription vertexAttributeDescriptions0[4] = {};
	vertexAttributeDescriptions0[0].location = 0; // Position
	vertexAttributeDescriptions0[0].binding = 0;
	vertexAttributeDescriptions0[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions0[0].offset = 0;
							   
	vertexAttributeDescriptions0[1].location = 1; // Normal
	vertexAttributeDescriptions0[1].binding = 0;
	vertexAttributeDescriptions0[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions0[1].offset = sizeof(glm::vec3);
							   
	vertexAttributeDescriptions0[2].location = 2; // Tangent
	vertexAttributeDescriptions0[2].binding = 0;
	vertexAttributeDescriptions0[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions0[2].offset = 2 * sizeof(glm::vec3);
							   
	vertexAttributeDescriptions0[3].location = 3; // Texture coordinate
	vertexAttributeDescriptions0[3].binding = 0;
	vertexAttributeDescriptions0[3].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions0[3].offset = 3 * sizeof(glm::vec3);

	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.pNext = nullptr;
	vertexInputState.flags = 0;
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputState.vertexAttributeDescriptionCount = 4;
	vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions0;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.pNext = nullptr;
	inputAssemblyState.flags = 0;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.pNext = nullptr;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasClamp = 0;
	rasterizationState.depthBiasConstantFactor = 0;
	rasterizationState.depthBiasSlopeFactor = 0;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT; // Cull back faces
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;

	// No multisampling
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.pNext = nullptr;
	multisampleState.flags = 0;
	multisampleState.pSampleMask = nullptr;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 0.0f;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.pNext = nullptr;
	depthStencilState.flags = 0;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.front.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.front.depthFailOp = VK_STENCIL_OP_ZERO;
	depthStencilState.front.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.front.compareMask = 0;
	depthStencilState.front.writeMask = 0;
	depthStencilState.front.reference = 0;
	depthStencilState.back = depthStencilState.front;
	depthStencilState.minDepthBounds = 0;
	depthStencilState.maxDepthBounds = 0;

	// No blending
	VkPipelineColorBlendAttachmentState attachmentBlendState = {};
	attachmentBlendState.colorWriteMask = 0xf;
	attachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	attachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	attachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	attachmentBlendState.blendEnable = VK_FALSE;
	attachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	attachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.pNext = nullptr;
	blendState.flags = 0;
	blendState.logicOpEnable = VK_FALSE;
	blendState.logicOp = VK_LOGIC_OP_NO_OP;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &attachmentBlendState;
	blendState.blendConstants[0] = 1.0f;
	blendState.blendConstants[1] = 1.0f;
	blendState.blendConstants[2] = 1.0f;
	blendState.blendConstants[3] = 1.0f;

	// Dynamic viewport and scissor to allow for window resizing
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = sizeof(mDirectionalLightingDescriptorSetLayouts)/sizeof(VkDescriptorSetLayout);
	pipelineLayoutCreateInfo.pSetLayouts = mDirectionalLightingDescriptorSetLayouts;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mDirectionalPipelineLayout);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pColorBlendState = &blendState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = mDirectionalPipelineLayout;
	pipelineCreateInfo.renderPass = mMainRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = 0;
	vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mDirectionalPipeline);

	// Create skybox pipeline
	shaderStages[0].module = mSkyboxVertexShader;
	shaderStages[1].module = mSkyboxFragmentShader;

	vertexBindingDescription.stride = sizeof(glm::vec3);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescription = {};
	vertexAttributeDescription.location = 0; // Position
	vertexAttributeDescription.binding = 0;
	vertexAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescription.offset = 0;

	vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputState.vertexAttributeDescriptionCount = 1;
	vertexInputState.pVertexAttributeDescriptions = &vertexAttributeDescription;

	rasterizationState.cullMode = VK_CULL_MODE_NONE;

	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mSkyboxDescriptorSetLayout;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mSkyboxPipelineLayout);

	pipelineCreateInfo.layout = mSkyboxPipelineLayout;
	pipelineCreateInfo.subpass = 1;
	vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mSkyboxPipeline);

	// Create convolute pipeline
	VkPipelineShaderStageCreateInfo convoluteShaderStages[3] = {};
	convoluteShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	convoluteShaderStages[0].pNext = nullptr;
	convoluteShaderStages[0].flags = 0;
	convoluteShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	convoluteShaderStages[0].module = mCubemapVertexShader;
	convoluteShaderStages[0].pName = "main";
	convoluteShaderStages[0].pSpecializationInfo = nullptr;

	convoluteShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	convoluteShaderStages[1].pNext = nullptr;
	convoluteShaderStages[1].flags = 0;
	convoluteShaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	convoluteShaderStages[1].module = mCubemapGeometryShader;
	convoluteShaderStages[1].pName = "main";
	convoluteShaderStages[1].pSpecializationInfo = nullptr;

	convoluteShaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	convoluteShaderStages[2].pNext = nullptr;
	convoluteShaderStages[2].flags = 0;
	convoluteShaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	convoluteShaderStages[2].module = mConvoluteFragmentShader;
	convoluteShaderStages[2].pName = "main";
	convoluteShaderStages[2].pSpecializationInfo = nullptr;

	VkViewport viewport = { 0, 0, IRRADIANCE_WIDTH_HEIGHT, IRRADIANCE_WIDTH_HEIGHT, 0.0f, 1.0f };
	VkRect2D scissor = { {0, 0}, { IRRADIANCE_WIDTH_HEIGHT, IRRADIANCE_WIDTH_HEIGHT } };

	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;

	dynamicState.dynamicStateCount = 0;
	dynamicState.pDynamicStates = nullptr;

	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mEnvironmentDescriptorSetLayout;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mConvolutePipelineLayout);

	pipelineCreateInfo.stageCount = 3;
	pipelineCreateInfo.pStages = convoluteShaderStages;
	pipelineCreateInfo.layout = mConvolutePipelineLayout;
	pipelineCreateInfo.renderPass = mEnvironmentRenderPass;
	pipelineCreateInfo.subpass = 0;
	vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mConvolutePipeline);

	// Create prefilter pipeline
	VkPipelineShaderStageCreateInfo prefilterShaderStages[3] = {};
	prefilterShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	prefilterShaderStages[0].pNext = nullptr;
	prefilterShaderStages[0].flags = 0;
	prefilterShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	prefilterShaderStages[0].module = mCubemapVertexShader;
	prefilterShaderStages[0].pName = "main";
	prefilterShaderStages[0].pSpecializationInfo = nullptr;

	prefilterShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	prefilterShaderStages[1].pNext = nullptr;
	prefilterShaderStages[1].flags = 0;
	prefilterShaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	prefilterShaderStages[1].module = mCubemapGeometryShader;
	prefilterShaderStages[1].pName = "main";
	prefilterShaderStages[1].pSpecializationInfo = nullptr;

	prefilterShaderStages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	prefilterShaderStages[2].pNext = nullptr;
	prefilterShaderStages[2].flags = 0;
	prefilterShaderStages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	prefilterShaderStages[2].module = mPrefilterFragmentShader;
	prefilterShaderStages[2].pName = "main";
	prefilterShaderStages[2].pSpecializationInfo = nullptr;

	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;

	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	mPrefilterViewport.x = 0;
	mPrefilterViewport.y = 0;
	mPrefilterViewport.minDepth = 0.0f;
	mPrefilterViewport.maxDepth = 1.0f;

	mPrefilterScissor.offset = { 0, 0 };

	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mEnvironmentDescriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	VkPushConstantRange pushConstant = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 }; // Roughness parameter push constant
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;
	vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mPrefilterPipelineLayout);

	pipelineCreateInfo.stageCount = 3;
	pipelineCreateInfo.pStages = prefilterShaderStages;
	pipelineCreateInfo.layout = mPrefilterPipelineLayout;
	pipelineCreateInfo.renderPass = mEnvironmentRenderPass;
	pipelineCreateInfo.subpass = 0;
	vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPrefilterPipeline);

	// Create 2D pipeline - used for sprites, buttons, and text
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = mQuadVertexShader;
	shaderStages[0].pName = "main";
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = m2DFragmentShader;
	shaderStages[1].pName = "main";
	shaderStages[1].pSpecializationInfo = nullptr;

	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(glm::vec3) + sizeof(glm::vec2);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescriptions1[2] = {};
	vertexAttributeDescriptions1[0].location = 0; // Position
	vertexAttributeDescriptions1[0].binding = 0;
	vertexAttributeDescriptions1[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions1[0].offset = 0;
							   
	vertexAttributeDescriptions1[1].location = 1; // Texture coordinate
	vertexAttributeDescriptions1[1].binding = 0;
	vertexAttributeDescriptions1[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions1[1].offset = sizeof(glm::vec3);

	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputState.vertexAttributeDescriptionCount = 2;
	vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions1;

	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;

	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasClamp = 0;
	rasterizationState.depthBiasConstantFactor = 0;
	rasterizationState.depthBiasSlopeFactor = 0;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.lineWidth = 1.0f;

	multisampleState.pSampleMask = nullptr;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 0.0f;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;

	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.front.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.front.depthFailOp = VK_STENCIL_OP_ZERO;
	depthStencilState.front.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.front.compareMask = 0;
	depthStencilState.front.writeMask = 0;
	depthStencilState.front.reference = 0;
	depthStencilState.back = depthStencilState.front;
	depthStencilState.minDepthBounds = 0;
	depthStencilState.maxDepthBounds = 0;

	// Blending
	attachmentBlendState.blendEnable = VK_TRUE;
	attachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;;
	attachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	attachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	attachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	attachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	attachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD;

	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.pNext = nullptr;
	blendState.flags = 0;
	blendState.logicOpEnable = VK_FALSE;
	blendState.logicOp = VK_LOGIC_OP_NO_OP;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &attachmentBlendState;
	blendState.blendConstants[0] = 1.0f;
	blendState.blendConstants[1] = 1.0f;
	blendState.blendConstants[2] = 1.0f;
	blendState.blendConstants[3] = 1.0f;

	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mImageDescriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 2;
	VkPushConstantRange pushConstants[2] = { { VK_SHADER_STAGE_VERTEX_BIT, 0, 64 }, { VK_SHADER_STAGE_FRAGMENT_BIT, 64, 80 } }; // Transform matrix, colour, and text 'mode' push constants
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants;
	vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &m2DPipelineLayout);

	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pColorBlendState = &blendState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = m2DPipelineLayout;
	pipelineCreateInfo.renderPass = mMainRenderPass;
	pipelineCreateInfo.subpass = 2;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = 0;
	vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m2DPipeline);
	#pragma endregion

	#pragma region Create command buffer
	mGraphicsCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	mGraphicsCommandPoolCreateInfo.pNext = nullptr;
	mGraphicsCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	mGraphicsCommandPoolCreateInfo.queueFamilyIndex = mGraphicsQueueIndex;
	vkCreateCommandPool(mDevice, &mGraphicsCommandPoolCreateInfo, nullptr, &mCommandPool);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = mCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &mCommandBuffer);

	mCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	mCommandBufferBeginInfo.pNext = nullptr;
	mCommandBufferBeginInfo.flags = 0;
	mCommandBufferBeginInfo.pInheritanceInfo = nullptr;
	#pragma endregion

	#pragma region Create cube vertex buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	// Create vertex buffer
	bufferRange = 36 * sizeof(glm::vec3);
	bufferCreateInfo.size = bufferRange;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mCubeVertexBuffer);

	vkGetBufferMemoryRequirements(mDevice, mCubeVertexBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mCubeVertexMemory);

	vkBindBufferMemory(mDevice, mCubeVertexBuffer, mCubeVertexMemory, 0);

	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

	vkGetBufferMemoryRequirements(mDevice, stagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &stagingMemory);

	vkBindBufferMemory(mDevice, stagingBuffer, stagingMemory, 0);

	glm::vec3 skyboxPositions[36] = {
		glm::vec3(-1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f),

		glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),

		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),

		glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),

		glm::vec3(-1.0f,  1.0f, -1.0f),
		glm::vec3(1.0f,  1.0f, -1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f),

		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f)
	};

	unsigned char* data;
	vkMapMemory(mDevice, stagingMemory, 0, bufferRange, 0, (void**)&data);
	memcpy(data, skyboxPositions, bufferRange);
	vkUnmapMemory(mDevice, stagingMemory);

	// Copy staging buffer into vertex buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferRange;
	vkCmdCopyBuffer(mCommandBuffer, stagingBuffer, mCubeVertexBuffer, 1, &copyRegion);

	vkEndCommandBuffer(mCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);

	vkFreeMemory(mDevice, stagingMemory, nullptr);
	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	#pragma endregion

	#pragma region Create UI Quad vertex buffer
	// Create vertex buffer
	float uiQuadVertices[] = {
	-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
	-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,
	 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
	 0.5f,  0.5f, 0.0f,   1.0f, 1.0f };

	bufferRange = sizeof(uiQuadVertices);
	bufferCreateInfo.size = bufferRange;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mQuadVertexBuffer);

	vkGetBufferMemoryRequirements(mDevice, mQuadVertexBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mQuadVertexMemory);

	vkBindBufferMemory(mDevice, mQuadVertexBuffer, mQuadVertexMemory, 0);

	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

	vkGetBufferMemoryRequirements(mDevice, stagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &stagingMemory);

	vkBindBufferMemory(mDevice, stagingBuffer, stagingMemory, 0);

	vkMapMemory(mDevice, stagingMemory, 0, bufferRange, 0, (void**)&data);
	memcpy(data, uiQuadVertices, bufferRange);
	vkUnmapMemory(mDevice, stagingMemory);

	// Copy staging buffer into vertex buffer
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferRange;
	vkCmdCopyBuffer(mCommandBuffer, stagingBuffer, mQuadVertexBuffer, 1, &copyRegion);

	vkEndCommandBuffer(mCommandBuffer);

	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);

	vkFreeMemory(mDevice, stagingMemory, nullptr);
	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	#pragma endregion

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;
	vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mImageAvailable);
	vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mRenderComplete);

	mViewport.x = 0;
	mViewport.y = 0;
	mViewport.width = mSurfaceWidth;
	mViewport.height = mSurfaceHeight;
	mViewport.minDepth = 0.0f;
	mViewport.maxDepth = 1.0f;

	mScissor.offset = { 0, 0 };
	mScissor.extent = { mSurfaceWidth, mSurfaceHeight };

	mRenderSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	mRenderSubmitInfo.pNext = nullptr;
	mRenderSubmitInfo.waitSemaphoreCount = 1;
	mRenderSubmitInfo.pWaitSemaphores = &mImageAvailable;
	mRenderSubmitInfo.pWaitDstStageMask = &mWaitFlags;
	mRenderSubmitInfo.commandBufferCount = 1;
	mRenderSubmitInfo.pCommandBuffers = &mCommandBuffer;
	mRenderSubmitInfo.signalSemaphoreCount = 1;
	mRenderSubmitInfo.pSignalSemaphores = &mRenderComplete;

	mPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	mPresentInfo.pNext = nullptr;
	mPresentInfo.waitSemaphoreCount = 1;
	mPresentInfo.pWaitSemaphores = &mRenderComplete;
	mPresentInfo.swapchainCount = 1;
	mPresentInfo.pSwapchains = &mSwapchain;
	mPresentInfo.pResults = nullptr;

	m2DProjection = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f / mSurfaceWidth, 2.0f / mSurfaceHeight, 1.0f));
}

void RenderSystem::start()
{
	Texture& brdfLUT = TextureManager::instance().getTexture({ "Images/brdfLUT.png", VK_FORMAT_R8G8B8A8_UNORM });
	VkDescriptorImageInfo brdfLUTImageInfo = {};
	brdfLUTImageInfo.sampler = brdfLUT.mSampler;
	brdfLUTImageInfo.imageView = brdfLUT.mImageView;
	brdfLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorSetWrite = {};
	descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrite.pNext = nullptr;
	descriptorSetWrite.dstSet = mCameraDescriptorSet;
	descriptorSetWrite.dstBinding = 3;
	descriptorSetWrite.dstArrayElement = 0;
	descriptorSetWrite.descriptorCount = 1;
	descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrite.pImageInfo = &brdfLUTImageInfo;
	descriptorSetWrite.pBufferInfo = nullptr;
	descriptorSetWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(mDevice, 1, &descriptorSetWrite, 0, nullptr);
}

void RenderSystem::allocateMeshBuffers(Mesh& mesh)
{
	// Free buffers if they have been allocated
	if (mesh._vertexBuffer || mesh._indexBuffer)
	{
		vkFreeMemory(mDevice, mesh._vertexMemory, nullptr);
		vkDestroyBuffer(mDevice, mesh._vertexBuffer, nullptr);
		vkUnmapMemory(mDevice, mesh._vertexStagingMemory);
		vkFreeMemory(mDevice, mesh._vertexStagingMemory, nullptr);
		vkDestroyBuffer(mDevice, mesh._vertexStagingBuffer, nullptr);
		vkFreeMemory(mDevice, mesh._indexMemory, nullptr);
		vkDestroyBuffer(mDevice, mesh._indexBuffer, nullptr);
		vkUnmapMemory(mDevice, mesh._indexStagingMemory);
		vkFreeMemory(mDevice, mesh._indexStagingMemory, nullptr);
		vkDestroyBuffer(mDevice, mesh._indexStagingBuffer, nullptr);
	}

	unsigned int localMemoryType = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	unsigned int hostMemoryType = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Create vertex buffer
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = mesh.nVertices * sizeof(Vertex);
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mesh._vertexBuffer);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(mDevice, mesh._vertexBuffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = localMemoryType;
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mesh._vertexMemory);

	vkBindBufferMemory(mDevice, mesh._vertexBuffer, mesh._vertexMemory, 0);

	// Create vertex staging buffer
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mesh._vertexStagingBuffer);

	vkGetBufferMemoryRequirements(mDevice, mesh._vertexStagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = hostMemoryType;
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mesh._vertexStagingMemory);

	vkBindBufferMemory(mDevice, mesh._vertexStagingBuffer, mesh._vertexStagingMemory, 0);

	vkMapMemory(mDevice, mesh._vertexStagingMemory, 0, bufferCreateInfo.size, 0, (void**)&mesh.vertices);

	// Create index buffer
	bufferCreateInfo.size = mesh.nIndices * sizeof(unsigned int);
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mesh._indexBuffer);

	vkGetBufferMemoryRequirements(mDevice, mesh._indexBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = localMemoryType;
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mesh._indexMemory);

	vkBindBufferMemory(mDevice, mesh._indexBuffer, mesh._indexMemory, 0);

	// Create index staging buffer
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mesh._indexStagingBuffer);

	vkGetBufferMemoryRequirements(mDevice, mesh._indexStagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = hostMemoryType;
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mesh._indexStagingMemory);

	vkBindBufferMemory(mDevice, mesh._indexStagingBuffer, mesh._indexStagingMemory, 0);

	vkMapMemory(mDevice, mesh._indexStagingMemory, 0, bufferCreateInfo.size, 0, (void**)&mesh.indices);
}

void RenderSystem::addMesh(const Entity& entity)
{
	Mesh& mesh = entity.getComponent<Mesh>();

	#pragma region Create mesh resources
	unsigned int localMemoryType = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	unsigned int hostMemoryType = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	mesh._vertexBuffer = VK_NULL_HANDLE;
	mesh._indexBuffer = VK_NULL_HANDLE;
	if (mesh.nVertices != 0 && mesh.nIndices != 0)
		allocateMeshBuffers(mesh);

	// Create uniform buffer - stores model and normal matrices
	unsigned int uniformBufferRange = sizeof(glm::mat4) * 2;
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = uniformBufferRange;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mesh._uniformBuffer);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(mDevice, mesh._uniformBuffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = localMemoryType;
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mesh._uniformMemory);

	vkBindBufferMemory(mDevice, mesh._uniformBuffer, mesh._uniformMemory, 0);

	// Create uniform staging buffer
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &mesh._uniformStagingBuffer);

	vkGetBufferMemoryRequirements(mDevice, mesh._uniformStagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = hostMemoryType;
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &mesh._uniformStagingMemory);

	vkBindBufferMemory(mDevice, mesh._uniformStagingBuffer, mesh._uniformStagingMemory, 0);

	vkMapMemory(mDevice, mesh._uniformStagingMemory, 0, bufferCreateInfo.size, 0, (void**)&mesh._uniformData);

	// Create descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mDirectionalLightingDescriptorSetLayouts[1];
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &mesh._descriptorSet);

	VkDescriptorBufferInfo uniformBufferInfo = {};
	uniformBufferInfo.buffer = mesh._uniformBuffer;
	uniformBufferInfo.offset = 0;
	uniformBufferInfo.range = uniformBufferRange;

	VkWriteDescriptorSet descriptorSetWrite = {};
	descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrite.pNext = nullptr;
	descriptorSetWrite.dstSet = mesh._descriptorSet;
	descriptorSetWrite.dstBinding = 0;
	descriptorSetWrite.dstArrayElement = 0;
	descriptorSetWrite.descriptorCount = 1;
	descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetWrite.pImageInfo = nullptr;
	descriptorSetWrite.pBufferInfo = &uniformBufferInfo;
	descriptorSetWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(mDevice, 1, &descriptorSetWrite, 0, nullptr);
	#pragma endregion

	if(mesh.material.albedo && mesh.material.normal && mesh.material.roughness && mesh.material.metalness && mesh.material.ambientOcclusion)
		mesh.updateMaterial();

	Transform& transform = entity.getComponent<Transform>();
	transform.subscribeChangedEvent(&mMeshTransformChangedCallback);
	meshTransformChanged(transform);

	mMeshIDs.push_back(entity.ID());
}

void RenderSystem::removeMesh(const std::vector<EntityID>::iterator& IDIterator)
{
	Mesh& mesh = mMeshManager.getComponent(*IDIterator);

	vkFreeMemory(mDevice, mesh._vertexMemory, nullptr);
	vkDestroyBuffer(mDevice, mesh._vertexBuffer, nullptr);
	vkUnmapMemory(mDevice, mesh._vertexStagingMemory);
	vkFreeMemory(mDevice, mesh._vertexStagingMemory, nullptr);
	vkDestroyBuffer(mDevice, mesh._vertexStagingBuffer, nullptr);
	vkFreeMemory(mDevice, mesh._indexMemory, nullptr);
	vkDestroyBuffer(mDevice, mesh._indexBuffer, nullptr);
	vkUnmapMemory(mDevice, mesh._indexStagingMemory);
	vkFreeMemory(mDevice, mesh._indexStagingMemory, nullptr);
	vkDestroyBuffer(mDevice, mesh._indexStagingBuffer, nullptr);
	vkFreeMemory(mDevice, mesh._uniformMemory, nullptr);
	vkDestroyBuffer(mDevice, mesh._uniformBuffer, nullptr);
	vkUnmapMemory(mDevice, mesh._uniformStagingMemory);
	vkFreeMemory(mDevice, mesh._uniformStagingMemory, nullptr);
	vkDestroyBuffer(mDevice, mesh._uniformStagingBuffer, nullptr);
	vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &mesh._descriptorSet);

	Transform& transform = mTransformManager.getComponent(*IDIterator);
	if(transform.changedCallbacks.data) // Incase the transform has already been freed
		transform.unsubscribeChangedEvent(&mMeshTransformChangedCallback);

	mMeshIDs.erase(IDIterator);
}

void RenderSystem::addDirectionalLight(const Entity& entity)
{
	DirectionalLight& directionalLight = entity.getComponent<DirectionalLight>();

	#pragma region Create directional light resources
	unsigned int uniformBufferRange = 2 * sizeof(glm::vec3) + 4; // vec3 has an alignment of 16 bytes according to std140
	// Create uniform buffer
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = uniformBufferRange;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &directionalLight._uniformBuffer);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(mDevice, directionalLight._uniformBuffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &directionalLight._uniformMemory);

	vkBindBufferMemory(mDevice, directionalLight._uniformBuffer, directionalLight._uniformMemory, 0);

	// Create uniform staging buffer
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(mDevice, &bufferCreateInfo, nullptr, &directionalLight._uniformStagingBuffer);

	vkGetBufferMemoryRequirements(mDevice, directionalLight._uniformStagingBuffer, &memoryRequirements);

	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeFromProperties(mPhysicalDeviceMemoryProperties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(mDevice, &memoryAllocateInfo, nullptr, &directionalLight._uniformStagingMemory);

	vkBindBufferMemory(mDevice, directionalLight._uniformStagingBuffer, directionalLight._uniformStagingMemory, 0);

	vkMapMemory(mDevice, directionalLight._uniformStagingMemory, 0, bufferCreateInfo.size, 0, (void**)&directionalLight._uniformData);

	// Create descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mDirectionalLightingDescriptorSetLayouts[2];
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &directionalLight._descriptorSet);

	VkDescriptorBufferInfo uniformBufferInfo = {};
	uniformBufferInfo.buffer = directionalLight._uniformBuffer;
	uniformBufferInfo.offset = 0;
	uniformBufferInfo.range = uniformBufferRange;

	VkWriteDescriptorSet descriptorSetWrite = {};
	descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrite.pNext = nullptr;
	descriptorSetWrite.dstSet = directionalLight._descriptorSet;
	descriptorSetWrite.dstBinding = 0;
	descriptorSetWrite.dstArrayElement = 0;
	descriptorSetWrite.descriptorCount = 1;
	descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetWrite.pImageInfo = nullptr;
	descriptorSetWrite.pBufferInfo = &uniformBufferInfo;
	descriptorSetWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(mDevice, 1, &descriptorSetWrite, 0, nullptr);
	#pragma endregion

	directionalLightChanged(directionalLight);

	Transform& transform = entity.getComponent<Transform>();
	transform.subscribeChangedEvent(&mDirectionalLightTransformChangedCallback);
	directionalLightTransformChanged(transform);

	mDirectionalLightIDs.push_back(entity.ID());
}

void RenderSystem::removeDirectionalLight(const std::vector<EntityID>::iterator& IDIterator)
{
	DirectionalLight& directionalLight = mDirectionalLightManager.getComponent(*IDIterator);

	vkFreeMemory(mDevice, directionalLight._uniformMemory, nullptr);
	vkDestroyBuffer(mDevice, directionalLight._uniformBuffer, nullptr);
	vkUnmapMemory(mDevice, directionalLight._uniformStagingMemory);
	vkFreeMemory(mDevice, directionalLight._uniformStagingMemory, nullptr);
	vkDestroyBuffer(mDevice, directionalLight._uniformStagingBuffer, nullptr);
	vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &directionalLight._descriptorSet);

	Transform& transform = mTransformManager.getComponent(*IDIterator);
	if(transform.changedCallbacks.data) // Incase the transform has already been freed
		transform.unsubscribeChangedEvent(&mDirectionalLightTransformChangedCallback);

	mDirectionalLightIDs.erase(IDIterator);
}

void RenderSystem::addSprite(const Entity& entity)
{
	Sprite& sprite = entity.getComponent<Sprite>();

    #pragma region Create button resources
	// Create descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mImageDescriptorSetLayout;
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &sprite._descriptorSet);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = sprite.texture->mSampler;
	imageInfo.imageView = sprite.texture->mImageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorSetWrite = {};
	descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrite.pNext = nullptr;
	descriptorSetWrite.dstBinding = 0;
	descriptorSetWrite.dstArrayElement = 0;
	descriptorSetWrite.descriptorCount = 1;
	descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrite.pBufferInfo = nullptr;
	descriptorSetWrite.pTexelBufferView = nullptr;
	descriptorSetWrite.dstSet = sprite._descriptorSet;
	descriptorSetWrite.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(mDevice, 1, &descriptorSetWrite, 0, nullptr);
    #pragma endregion

	mSpriteIDs.push_back(entity.ID());
}

void RenderSystem::removeSprite(const std::vector<EntityID>::iterator& IDIterator)
{
	EntityID ID = *IDIterator;
	Sprite& sprite = mSpriteManager.getComponent(ID);
	vkFreeDescriptorSets(mDevice, mDescriptorPool, 1, &sprite._descriptorSet);

	mSpriteIDs.erase(std::find(mSpriteIDs.begin(), mSpriteIDs.end(), ID));
}

void RenderSystem::addUIButton(const Entity& entity)
{
	UIButton& uiButton = entity.getComponent<UIButton>();

	#pragma region Create button resources
	// Create descriptor sets
	VkDescriptorSet sets[3];

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = nullptr;
	descriptorSetAllocateInfo.descriptorPool = mDescriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &mImageDescriptorSetLayout;
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &sets[0]);
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &sets[1]);
	vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &sets[2]);

	VkDescriptorImageInfo imageInfos[3] = {};
	imageInfos[0].sampler = uiButton.unpressedTexture->mSampler;
	imageInfos[0].imageView = uiButton.unpressedTexture->mImageView;
	imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfos[1].sampler = uiButton.canpressTexture->mSampler;
	imageInfos[1].imageView = uiButton.canpressTexture->mImageView;
	imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfos[2].sampler = uiButton.pressedTexture->mSampler;
	imageInfos[2].imageView = uiButton.pressedTexture->mImageView;
	imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorSetWrites[3] = {};
	descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[0].pNext = nullptr;
	descriptorSetWrites[0].dstBinding = 0;
	descriptorSetWrites[0].dstArrayElement = 0;
	descriptorSetWrites[0].descriptorCount = 1;
	descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[0].pBufferInfo = nullptr;
	descriptorSetWrites[0].pTexelBufferView = nullptr;
	descriptorSetWrites[2] = descriptorSetWrites[1] = descriptorSetWrites[0];
	descriptorSetWrites[0].dstSet = sets[0];
	descriptorSetWrites[0].pImageInfo = &imageInfos[0];
	descriptorSetWrites[1].dstSet = sets[1];
	descriptorSetWrites[1].pImageInfo = &imageInfos[1];
	descriptorSetWrites[2].dstSet = sets[2];
	descriptorSetWrites[2].pImageInfo = &imageInfos[2];

	vkUpdateDescriptorSets(mDevice, 3, descriptorSetWrites, 0, nullptr);

	uiButton._unpressedDescriptorSet = sets[0];
	uiButton._canpressDescriptorSet = sets[1];
	uiButton._pressedDescriptorSet = sets[2];
	#pragma endregion

	mUIButtonIDs.push_back(entity.ID());
}

void RenderSystem::removeUIButton(const std::vector<EntityID>::iterator& IDIterator)
{
	EntityID ID = *IDIterator;
	UIButton& uiButton = mUIButtonManager.getComponent(ID);

	VkDescriptorSet descriptorSets[3] = { uiButton._unpressedDescriptorSet, uiButton._canpressDescriptorSet, uiButton._pressedDescriptorSet };
	vkFreeDescriptorSets(mDevice, mDescriptorPool, 3, descriptorSets);

	mUIButtonIDs.erase(std::find(mUIButtonIDs.begin(), mUIButtonIDs.end(), ID));
}

RenderSystem& RenderSystem::instance()
{
	static RenderSystem instance;
	return instance;
}

RenderSystem::~RenderSystem()
{
	mTransformManager.unsubscribeAddedEvent(&mTransformAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mTransformRemovedCallback);
	mMeshManager.unsubscribeAddedEvent(&mMeshAddedCallback);
	mMeshManager.unsubscribeRemovedEvent(&mMeshRemovedCallback);
	mDirectionalLightManager.unsubscribeAddedEvent(&mDirectionalLightAddedCallback);
	mDirectionalLightManager.unsubscribeRemovedEvent(&mDirectionalLightRemovedCallback);
	mTransform2DManager.unsubscribeAddedEvent(&mTransform2DAddedCallback);
	mTransform2DManager.unsubscribeRemovedEvent(&mTransform2DRemovedCallback);
	mUITextManager.unsubscribeAddedEvent(&mUITextAddedCallback);
	mUITextManager.unsubscribeRemovedEvent(&mUITextRemovedCallback);
	mUIButtonManager.unsubscribeAddedEvent(&mUIButtonAddedCallback);
	mUIButtonManager.unsubscribeRemovedEvent(&mUIButtonRemovedCallback);
	mWindowManager.unsubscribeLMBPressedEvent(&mLMBPressedCallback);

	vkDestroySemaphore(mDevice, mRenderComplete, nullptr);
	vkDestroySemaphore(mDevice, mImageAvailable, nullptr);
	vkFreeMemory(mDevice, mQuadVertexMemory, nullptr);
	vkDestroyBuffer(mDevice, mQuadVertexBuffer, nullptr);
	vkFreeMemory(mDevice, mCubeVertexMemory, nullptr);
	vkDestroyBuffer(mDevice, mCubeVertexBuffer, nullptr);
	vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mCommandBuffer);
	vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	vkDestroyPipeline(mDevice, m2DPipeline, nullptr);
	vkDestroyPipeline(mDevice, mPrefilterPipeline, nullptr);
	vkDestroyPipeline(mDevice, mConvolutePipeline, nullptr);
	vkDestroyPipeline(mDevice, mSkyboxPipeline, nullptr);
	vkDestroyPipeline(mDevice, mDirectionalPipeline, nullptr);
	vkDestroyPipelineLayout(mDevice, m2DPipelineLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mPrefilterPipelineLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mConvolutePipelineLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mSkyboxPipelineLayout, nullptr);
	vkDestroyPipelineLayout(mDevice, mDirectionalPipelineLayout, nullptr);
	vkDestroyShaderModule(mDevice, mPrefilterFragmentShader, nullptr);
	vkDestroyShaderModule(mDevice, mConvoluteFragmentShader, nullptr);
	vkDestroyShaderModule(mDevice, mCubemapGeometryShader, nullptr);
	vkDestroyShaderModule(mDevice, mCubemapVertexShader, nullptr);
	vkDestroyShaderModule(mDevice, mSkyboxFragmentShader, nullptr);
	vkDestroyShaderModule(mDevice, mSkyboxVertexShader, nullptr);
	vkDestroyShaderModule(mDevice, mFragmentShader, nullptr);
	vkDestroyShaderModule(mDevice, mVertexShader, nullptr);
	vkDestroyShaderModule(mDevice, mQuadVertexShader, nullptr);
	vkDestroyShaderModule(mDevice, m2DFragmentShader, nullptr);
	VkDescriptorSet descriptorSets[3] = { mCameraDescriptorSet, mSkyboxDescriptorSet, mEnvironmentDescriptorSet };
	vkFreeDescriptorSets(mDevice, mDescriptorPool, 3, descriptorSets);
	vkDestroyDescriptorSetLayout(mDevice, mImageDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mEnvironmentDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mSkyboxDescriptorSetLayout, nullptr);
	for (unsigned int i = 0; i < sizeof(mDirectionalLightingDescriptorSetLayouts) / sizeof(VkDescriptorSetLayout); i++)
		vkDestroyDescriptorSetLayout(mDevice, mDirectionalLightingDescriptorSetLayouts[i], nullptr);
	vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
	vkUnmapMemory(mDevice, mCameraUniformMemory);
	vkFreeMemory(mDevice, mCameraUniformMemory, nullptr);
	vkDestroyBuffer(mDevice, mCameraUniformBuffer, nullptr);
	for (unsigned int i = 0; i < mNPrefilterMips; i++)
		vkDestroyFramebuffer(mDevice, mPrefilterFramebuffers[i], nullptr);
	vkDestroyFramebuffer(mDevice, mConvoluteFramebuffer, nullptr);
	for (unsigned int i = 0; i < mNSwapchainImages; i++)
		vkDestroyFramebuffer(mDevice, mFramebuffers[i], nullptr);
	vkDestroyRenderPass(mDevice, mEnvironmentRenderPass, nullptr);
	vkDestroyRenderPass(mDevice, mMainRenderPass, nullptr);
	vkDestroySampler(mDevice, mPrefilterSampler, nullptr);
	vkDestroyImageView(mDevice, mPrefilterImageView, nullptr);
	for (unsigned int i = 0; i < mNPrefilterMips; i++)
		vkDestroyImageView(mDevice, mPrefilterImageViews[i], nullptr);
	vkFreeMemory(mDevice, mPrefilterMemory, nullptr);
	vkDestroyImage(mDevice, mPrefilterImage, nullptr);
	vkDestroySampler(mDevice, mIrradianceSampler, nullptr);
	vkDestroyImageView(mDevice, mIrradianceImageView, nullptr);
	vkFreeMemory(mDevice, mIrradianceMemory, nullptr);
	vkDestroyImage(mDevice, mIrradianceImage, nullptr);
	vkDestroyImageView(mDevice, mDepthImageView, nullptr);
	vkFreeMemory(mDevice, mDepthMemory, nullptr);
	vkDestroyImage(mDevice, mDepthImage, nullptr);
	for (unsigned int i = 0; i < mNSwapchainImages; i++)
		vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
	vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
	vkDestroyDevice(mDevice, nullptr);
	vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
	vkDestroyInstance(mVkInstance, nullptr);
	delete[] mPhysicalDevices, mSwapchainImageViews, mFramebuffers, mPrefilterImageViews, mPrefilterFramebuffers;
}

void RenderSystem::transformAdded(const Entity& entity)
{
	const Composition& entityComposition = entity.composition();
	if ((entityComposition & mMeshComposition) == mMeshComposition)
		addMesh(entity);
	else if ((entityComposition & mDirectionalLightComposition) == mDirectionalLightComposition)
		addDirectionalLight(entity);
}

void RenderSystem::transformRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mMeshIDs.begin(), mMeshIDs.end(), entity.ID());
	if (IDIterator != mMeshIDs.end())
		removeMesh(IDIterator);
	IDIterator = std::find(mDirectionalLightIDs.begin(), mDirectionalLightIDs.end(), entity.ID());
	if (IDIterator != mDirectionalLightIDs.end())
		removeDirectionalLight(IDIterator);
}

void RenderSystem::meshAdded(const Entity& entity)
{
	if ((entity.composition() & mMeshComposition) == mMeshComposition)
		addMesh(entity);
}

void RenderSystem::meshRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mMeshIDs.begin(), mMeshIDs.end(), entity.ID());
	if (IDIterator != mMeshIDs.end())
		removeMesh(IDIterator);
}

void RenderSystem::directionalLightAdded(const Entity& entity)
{
	if ((entity.composition() & mDirectionalLightComposition) == mDirectionalLightComposition)
		addDirectionalLight(entity);
}

void RenderSystem::directionalLightRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mDirectionalLightIDs.begin(), mDirectionalLightIDs.end(), entity.ID());
	if (IDIterator != mDirectionalLightIDs.end())
		removeDirectionalLight(IDIterator);
}

void RenderSystem::transform2DAdded(const Entity& entity)
{
	const Composition& entityComposition = entity.composition();
	if ((entityComposition & mSpriteComposition) == mSpriteComposition)
		addSprite(entity);
	else if ((entityComposition & mUITextComposition) == mUITextComposition)
		mUITextIDs.push_back(entity.ID());
	else if ((entityComposition & mUIButtonComposition) == mUIButtonComposition)
		addUIButton(entity);
}

void RenderSystem::transform2DRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mSpriteIDs.begin(), mSpriteIDs.end(), entity.ID());
	if (IDIterator != mSpriteIDs.end())
	{
		removeSprite(IDIterator);
		return;
	}
	IDIterator = std::find(mUITextIDs.begin(), mUITextIDs.end(), entity.ID());
	if (IDIterator != mUITextIDs.end())
	{
		mUITextIDs.erase(IDIterator);
		return;
	}
	IDIterator = std::find(mUIButtonIDs.begin(), mUIButtonIDs.end(), entity.ID());
	if (IDIterator != mUIButtonIDs.end())
		removeUIButton(IDIterator);
}

void RenderSystem::spriteAdded(const Entity& entity)
{
	if ((entity.composition() & mSpriteComposition) == mSpriteComposition)
		addSprite(entity);
}

void RenderSystem::spriteRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mSpriteIDs.begin(), mSpriteIDs.end(), entity.ID());
	if (IDIterator != mSpriteIDs.end())
		removeSprite(IDIterator);
}

void RenderSystem::UITextAdded(const Entity& entity)
{
	if((entity.composition() & mUITextComposition) == mUITextComposition)
		mUITextIDs.push_back(entity.ID());
}

void RenderSystem::UITextRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mUITextIDs.begin(), mUITextIDs.end(), entity.ID());
	if(IDIterator != mUITextIDs.end())
		mUITextIDs.erase(IDIterator);
}

void RenderSystem::UIButtonAdded(const Entity& entity)
{
	if ((entity.composition() & mUIButtonComposition) == mUIButtonComposition)
		addUIButton(entity);
}

void RenderSystem::UIButtonRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mUIButtonIDs.begin(), mUIButtonIDs.end(), entity.ID());
	if (IDIterator != mUIButtonIDs.end())
		removeUIButton(IDIterator);
}

void RenderSystem::meshTransformChanged(Transform& transform) const
{
	// Update GPU side model and normal matrices
	Mesh& mesh = mMeshManager.getComponent(transform.entityID);
	memcpy(mesh._uniformData, &transform.matrix, sizeof(glm::mat4));
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(transform.matrix));
	memcpy(mesh._uniformData + sizeof(glm::mat4), &normalMatrix, sizeof(glm::mat4));

	#pragma region Copy staging buffer to device local buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(glm::mat4) * 2;
	vkCmdCopyBuffer(mCommandBuffer, mesh._uniformStagingBuffer, mesh._uniformBuffer, 1, &copyRegion);

	vkEndCommandBuffer(mCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);
	#pragma endregion
}

void RenderSystem::directionalLightTransformChanged(Transform& transform) const
{
	// Update GPU side direction vector
	DirectionalLight& directionalLight = mDirectionalLightManager.getComponent(transform.entityID);
	glm::vec3 direction = transform.worldDirection();
	memcpy(directionalLight._uniformData + sizeof(glm::vec3), &direction, sizeof(glm::vec3));

	#pragma region Copy staging buffer to device local buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = sizeof(glm::vec3);
	copyRegion.dstOffset = 16; // vec3 requires 12 bytes but it's alignment is 16 bytes
	copyRegion.size = sizeof(glm::vec3);
	vkCmdCopyBuffer(mCommandBuffer, directionalLight._uniformStagingBuffer, directionalLight._uniformBuffer, 1, &copyRegion);

	vkEndCommandBuffer(mCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);
	#pragma endregion
}

void RenderSystem::directionalLightChanged(const DirectionalLight& directionalLight)
{
	// Update GPU side light colour
	memcpy(directionalLight._uniformData, &directionalLight.colour, sizeof(glm::vec3));

	#pragma region Copy staging buffer to device local buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(glm::vec3);
	vkCmdCopyBuffer(mCommandBuffer, directionalLight._uniformStagingBuffer, directionalLight._uniformBuffer, 1, &copyRegion);

	vkEndCommandBuffer(mCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);
	#pragma endregion
}

void RenderSystem::cameraProjectionChanged(const Camera& camera)
{
	memcpy(mUniformData, &camera.projectionMatrix, sizeof(glm::mat4));
}

void RenderSystem::cameraViewChanged(const Transform& transform, const Camera& camera)
{
	memcpy(mUniformData + sizeof(glm::mat4), &camera.viewMatrix, sizeof(glm::mat4));
	if (transform.positionChanged)
		memcpy(mUniformData + 2 * sizeof(glm::mat4), &transform.worldPosition, sizeof(glm::vec3));
}

void RenderSystem::LMBPressed()
{
	glm::vec2 cursor = mWindowManager.cursorPosition();
	cursor -= glm::vec2(float(mSurfaceWidth) / 2.0f, float(mSurfaceHeight) / 2.0f);

	for (const EntityID& entity : mUIButtonIDs)
	{
		UIButton& uiButton = mUIButtonManager.getComponent(entity);
		if (uiButton._underCursor)
		{
			if(uiButton.toggle)
				uiButton._pressed = !uiButton._pressed;
				
			uiButton.callback(entity);
		}
	}
}

void RenderSystem::update()
{
	/* TEMPORARY FIX: FONT MUST BE LOADED BEFORE RENDER PASS */
	const Font* font = nullptr;
	if (!mUITextIDs.empty())
		font = &FontManager::instance().getFont(mUITextManager.getComponent(mUITextIDs[0]).font.c_str());

	glm::vec2 cursor = mWindowManager.cursorPosition();
	cursor -= glm::vec2(float(mSurfaceWidth) / 2.0f, float(mSurfaceHeight) / 2.0f);

	vkBeginCommandBuffer(mCommandBuffer, &mCommandBufferBeginInfo);

	/* --== MAIN RENDER PASS ==-- */
	vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, mImageAvailable, VK_NULL_HANDLE, &mCurrentImage);

	mRenderPassBeginInfo.framebuffer = mFramebuffers[mCurrentImage];
	vkCmdBeginRenderPass(mCommandBuffer, &mRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(mCommandBuffer, 0, 1, &mViewport);
	vkCmdSetScissor(mCommandBuffer, 0, 1, &mScissor);

	vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDirectionalPipeline);

	vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDirectionalPipelineLayout, 0, 1, &mCameraDescriptorSet, 0, nullptr);
	
	for (const EntityID& directionalLightID : mDirectionalLightIDs)
	{
		vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDirectionalPipelineLayout, 2, 1, &mDirectionalLightManager.getComponent(directionalLightID)._descriptorSet, 0, nullptr);

		for (const EntityID& meshID : mMeshIDs)
		{
			Mesh& mesh = mMeshManager.getComponent(meshID);
			vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDirectionalPipelineLayout, 1, 1, &mesh._descriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mesh._vertexBuffer, &ZERO_OFFSET);
			vkCmdBindIndexBuffer(mCommandBuffer, mesh._indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(mCommandBuffer, mesh.nIndices, 1, 0, 0, 0);
		}
	}

	// Render skybox
	vkCmdNextSubpass(mCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipeline);

	vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipelineLayout, 0, 1, &mSkyboxDescriptorSet, 0, nullptr);

	vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mCubeVertexBuffer, &ZERO_OFFSET);

	vkCmdDraw(mCommandBuffer, 36, 1, 0, 0);

	// Render UI
	vkCmdNextSubpass(mCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipeline);

	vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mQuadVertexBuffer, &ZERO_OFFSET);

	// Sprites
	unsigned int text = false;
	vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 76, sizeof(unsigned int), &text);
	for (const EntityID& entity : mSpriteIDs)
	{
		Transform2D& transform = mTransform2DManager.getComponent(entity);
		Sprite& sprite = mSpriteManager.getComponent(entity);

		glm::mat4 matrix = m2DProjection * transform.matrix * glm::scale(glm::mat4(1.0f), glm::vec3(sprite.width, sprite.height, 1.0f));
		vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &matrix[0][0]);
		glm::vec3 colour(1.0f);
		vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(glm::vec3), &colour);

		vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &sprite._descriptorSet, 0, nullptr);
		vkCmdDraw(mCommandBuffer, 4, 1, 0, 0);
	}

	// Buttons
	for (const EntityID& entity : mUIButtonIDs)
	{
		Transform2D& transform = mTransform2DManager.getComponent(entity);

		UIButton& uiButton = mUIButtonManager.getComponent(entity);

		glm::mat4 transformMatrix = transform.matrix * glm::scale(glm::mat4(1.0f), glm::vec3(uiButton.width, uiButton.height, 1.0f));
		glm::mat4 matrix = m2DProjection * transformMatrix;
		vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &matrix[0][0]);
		vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(glm::vec3), &uiButton.colour);

		// Detect if cursor is on button rectangle
		bool cursorOnButton = false;
		if (transform.worldRotation == 0.0f)
		{
			glm::vec2 halfExtent = 0.5f * glm::vec2(uiButton.width, uiButton.height);
			cursorOnButton = cursor.x > transform.worldPosition.x - halfExtent.x && cursor.x < transform.worldPosition.x + halfExtent.x && cursor.y < transform.worldPosition.y + halfExtent.y && cursor.y > transform.worldPosition.y - halfExtent.y;
		}
		else
		{
			glm::vec2 A = glm::vec2(transformMatrix * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));
			glm::vec2 B = glm::vec2(transformMatrix * glm::vec4(0.5f, -0.5f, 0.0f, 1.0f));
			glm::vec2 D = glm::vec2(transformMatrix * glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f));

			glm::vec2 AM = cursor - A;
			glm::vec2 AB = B - A;
			glm::vec2 AD = D - A;

			float d1 = glm::dot(AM, AB);
			float d2 = glm::dot(AM, AD);
			cursorOnButton = (0.0f < d1 && d1 < glm::dot(AB, AB)) && (0.0f < d2 && d2 < glm::dot(AD, AD));
		}
		uiButton._underCursor = cursorOnButton;
		
		// Check if the button is pressed and set the image accordingly
		if (!uiButton.toggle)
		{
			if (cursorOnButton)
			{
				if (mWindowManager.LMBDown())
				{
					uiButton._pressed = true;
					vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &uiButton._pressedDescriptorSet, 0, nullptr);
				}
				else
				{
					uiButton._pressed = false;
					vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &uiButton._canpressDescriptorSet, 0, nullptr);
				}
			}
			else
			{
				uiButton._pressed = false;
				vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &uiButton._unpressedDescriptorSet, 0, nullptr);
			}
		}
		else
		{
			if (uiButton._pressed)
				vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &uiButton._pressedDescriptorSet, 0, nullptr);
			else if(cursorOnButton)
				vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &uiButton._canpressDescriptorSet, 0, nullptr);
			else
				vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &uiButton._unpressedDescriptorSet, 0, nullptr);
		}

		vkCmdDraw(mCommandBuffer, 4, 1, 0, 0);
	}

	// Text
	text = true;
	vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 76, sizeof(unsigned int), &text);

	for (const EntityID& entity : mUITextIDs)
	{
		Transform2D& transform = mTransform2DManager.getComponent(entity);
		UIText& uiText = mUITextManager.getComponent(entity);

		glm::vec2 glyphOffset = transform.worldPosition;
		glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(transform.worldRotation), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::vec2 advanceDirection = transform.worldRotation == 0 ? glm::vec2(transform.scale.x, 0.0f) : glm::vec2(rotMatrix * glm::vec4(transform.scale.x, 0.0f, 0.0f, 1.0f));
		for (unsigned int i = 0; i < uiText.text.size(); i++)
		{
			Glyph& glyph = *font->at(uiText.text[i]);

			if (uiText.text[i] != 32)
			{
				glm::vec2 glyphSize(float(glyph.size.x) * transform.scale.x, float(glyph.size.y) * transform.scale.y);
				glm::vec2 glyphPosition = glyphOffset + glm::vec2(rotMatrix * glm::vec4(glyphSize.x/2.0f + transform.scale.x * float(glyph.bearing.x), -glyphSize.y/2.0f + transform.scale.y * float(glyph.size.y - glyph.bearing.y), 0.0f, 1.0f));
				glm::mat4 transformMatrix = m2DProjection * glm::translate(glm::mat4(1.0f), glm::vec3(glyphPosition, 0.0f)) * rotMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(glyphSize, 1.0f));

				vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transformMatrix[0][0]);
				vkCmdPushConstants(mCommandBuffer, m2DPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(glm::vec3), &uiText.colour);

				vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m2DPipelineLayout, 0, 1, &glyph.descriptorSet, 0, nullptr);

				vkCmdDraw(mCommandBuffer, 4, 1, 0, 0);
			}

			glyphOffset += float(glyph.advance >> 6) * advanceDirection;
		}
	}

	vkCmdEndRenderPass(mCommandBuffer);
	vkEndCommandBuffer(mCommandBuffer);

	vkQueueSubmit(mGraphicsQueue, 1, &mRenderSubmitInfo, VK_NULL_HANDLE);

	mPresentInfo.pImageIndices = &mCurrentImage;
	vkQueuePresentKHR(mPresentQueue, &mPresentInfo);

	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);
}

void RenderSystem::setCamera(const Entity& entity)
{
	if (mCamera)
	{
		Camera& camera = mCameraManager.getComponent(mCamera);
		camera.unsubscribeProjectionChangedEvent(&mProjectionChangedCallback);
		camera.unsubscribeViewChangedEvent(&mViewChangedCallback);
	}
	Camera& camera = entity.getComponent<Camera>();
	camera.subscribeProjectionChangedEvent(&mProjectionChangedCallback);
	camera.subscribeViewChangedEvent(&mViewChangedCallback);
	cameraProjectionChanged(camera);
	cameraViewChanged(entity.getComponent<Transform>(), camera);
	mCamera = entity.ID();
}

void RenderSystem::setSkybox(const Cubemap* cubemap)
{
	#pragma region Update descriptor set
	VkDescriptorImageInfo skyboxInfo = {};
	skyboxInfo.sampler = cubemap->mSampler;
	skyboxInfo.imageView = cubemap->mImageView;
	skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorSetWrites[2] = {};
	descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[0].pNext = nullptr;
	descriptorSetWrites[0].dstSet = mSkyboxDescriptorSet;
	descriptorSetWrites[0].dstBinding = 1;
	descriptorSetWrites[0].dstArrayElement = 0;
	descriptorSetWrites[0].descriptorCount = 1;
	descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[0].pImageInfo = &skyboxInfo;
	descriptorSetWrites[0].pBufferInfo = nullptr;
	descriptorSetWrites[0].pTexelBufferView = nullptr;

	descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSetWrites[1].pNext = nullptr;
	descriptorSetWrites[1].dstSet = mEnvironmentDescriptorSet;
	descriptorSetWrites[1].dstBinding = 0;
	descriptorSetWrites[1].dstArrayElement = 0;
	descriptorSetWrites[1].descriptorCount = 1;
	descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetWrites[1].pImageInfo = &skyboxInfo;
	descriptorSetWrites[1].pBufferInfo = nullptr;
	descriptorSetWrites[1].pTexelBufferView = nullptr;
	vkUpdateDescriptorSets(mDevice, 2, descriptorSetWrites, 0, nullptr);
	#pragma endregion

	/* --== RENDER LIGHTING MAPS ==-- */

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = 0;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	
	vkBeginCommandBuffer(mCommandBuffer, &mCommandBufferBeginInfo);

	vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &mCubeVertexBuffer, &ZERO_OFFSET);

	// Convolute
	vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mConvolutePipeline);

	vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mConvolutePipelineLayout, 0, 1, &mEnvironmentDescriptorSet, 0, nullptr);
	
	vkCmdBeginRenderPass(mCommandBuffer, &mConvoluteRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdDraw(mCommandBuffer, 36, 1, 0, 0);

	vkCmdEndRenderPass(mCommandBuffer);

	// Prefilter - renders each mip level with increasing roughness
	vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPrefilterPipeline);

	vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPrefilterPipelineLayout, 0, 1, &mEnvironmentDescriptorSet, 0, nullptr);

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = mEnvironmentRenderPass;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &mEnvironmentClearColour;

	for (unsigned int i = 0; i < mNPrefilterMips; i++)
	{
		renderPassBeginInfo.framebuffer = mPrefilterFramebuffers[i];

		unsigned int mipWidthHeight = (float)PREFILTER_WIDTH_HEIGHT * std::pow(0.5, i);
		renderPassBeginInfo.renderArea = { {0, 0}, {mipWidthHeight, mipWidthHeight} };
		mPrefilterViewport.width = mipWidthHeight;
		mPrefilterViewport.height = mipWidthHeight;
		mPrefilterScissor.extent = { mipWidthHeight, mipWidthHeight };

		vkCmdSetViewport(mCommandBuffer, 0, 1, &mPrefilterViewport);
		vkCmdSetScissor(mCommandBuffer, 0, 1, &mPrefilterScissor);

		vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		float roughness = (float)i / (float)(mNPrefilterMips - 1);
		vkCmdPushConstants(mCommandBuffer, mPrefilterPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &roughness);
		
		vkCmdDraw(mCommandBuffer, 36, 1, 0, 0);
		vkCmdEndRenderPass(mCommandBuffer);
	}

	vkEndCommandBuffer(mCommandBuffer);

	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mGraphicsQueue);
	vkResetCommandBuffer(mCommandBuffer, 0);
}

/* MODEL LOADING */

std::vector<std::string> getTextures(const aiMaterial* aiMaterial, const aiTextureType& type, const std::string& folder)
{
	unsigned int textureCount = aiMaterial->GetTextureCount(type);
	std::vector<std::string> textures(textureCount);
	aiString localDirectory;
	for (unsigned int i = 0; i < textureCount; i++)
	{
		aiMaterial->GetTexture(type, i, &localDirectory);
		textures.push_back(folder + "/" + localDirectory.C_Str());
	}
	return textures;
}

const Entity processMesh(const aiMesh* aiMesh, const aiScene* aiScene, const std::string& folder)
{
	const Entity meshEntity(aiMesh->mName.C_Str());
	meshEntity.addComponent<Transform>(TransformCreateInfo{});

	aiMaterial* aiMaterial = aiScene->mMaterials[aiMesh->mMaterialIndex];

	MaterialCreateInfo material = {};
	std::vector<std::string> textures = getTextures(aiMaterial, aiTextureType_DIFFUSE, folder);
	if (textures.empty())
	{
		material.albedo = "Images/default albedo.png";
		std::cout << "[NOTIFICATION] No albedo textures found for " << aiMesh->mName.C_Str() << " using default" << std::endl;
	}
	else
		material.albedo = textures[0];

	textures = getTextures(aiMaterial, aiTextureType_NORMALS, folder);
	if (textures.empty())
	{
		material.normal = "Images/default normal.png";
		std::cout << "[NOTIFICATION] No normal textures found for " << aiMesh->mName.C_Str() << " using default" << std::endl;
	}
	else
		material.normal = textures[0];

	textures = getTextures(aiMaterial, aiTextureType_SHININESS, folder);
	if (textures.empty())
	{
		material.roughness = "Images/default roughness.png";
		std::cout << "[NOTIFICATION] No roughness textures found for " << aiMesh->mName.C_Str() << " using default" << std::endl;
	}
	else
		material.roughness = textures[0];

	textures = getTextures(aiMaterial, aiTextureType_REFLECTION, folder);
	if (textures.empty())
	{
		material.metalness = "Images/default metalness.png";
		std::cout << "[NOTIFICATION] No metalness textures found for " << aiMesh->mName.C_Str() << " using default" << std::endl;
	}	
	else
		material.metalness = textures[0];

	textures = getTextures(aiMaterial, aiTextureType_AMBIENT, folder);
	if (textures.empty())
	{
		material.ambientOcclusion = "Images/default ambient occlusion.png";
		std::cout << "[NOTIFICATION] No ambient occlusion textures found for " << aiMesh->mName.C_Str() << " using default" << std::endl;
	}
	else
		material.ambientOcclusion = textures[0];
	
	Mesh& mesh = meshEntity.addComponent<Mesh>(Mesh{ aiMesh->mNumVertices, aiMesh->mNumFaces * 3, material });

	for (unsigned int i = 0; i < aiMesh->mNumVertices; i++)
	{
		mesh.vertices[i].position = glm::vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);
		mesh.vertices[i].normal = glm::vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
		mesh.vertices[i].tangent = glm::vec3(aiMesh->mTangents[i].x, aiMesh->mTangents[i].y, aiMesh->mTangents[i].z);
		mesh.vertices[i].textureCoordinate = glm::vec2(aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y); // Each vertex can have multiple texture coordinates; only use first channel
	}

	for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
			mesh.indices[i * 3 + j] = aiMesh->mFaces[i].mIndices[j];
	}

	mesh.updateBuffers();

	return meshEntity;
}

const Entity processNode(const aiNode* aiNode, const aiScene* aiScene, const std::string& folder)
{
	const Entity nodeEntity(aiNode->mName.C_Str());
	aiVector3D position, rotation, scale;
	aiNode->mTransformation.Decompose(scale, rotation, position);
	nodeEntity.addComponent<Transform>(TransformCreateInfo{ glm::vec3(position.x, position.y, position.z), glm::vec3(rotation.x, rotation.y, rotation.z), glm::vec3(scale.x, scale.y, scale.z) });

	for (unsigned int i = 0; i < aiNode->mNumMeshes; i++)
	{
		const Entity child = processMesh(aiScene->mMeshes[aiNode->mMeshes[i]], aiScene, folder);
		Transform& transform = nodeEntity.getComponent<Transform>();
		transform.addChild(child);
	}

	for (unsigned int i = 0; i < aiNode->mNumChildren; i++)
	{
		const Entity child = processNode(aiNode->mChildren[i], aiScene, folder);
		Transform& transform = nodeEntity.getComponent<Transform>();
		transform.addChild(child);
	}

	return nodeEntity;
}

Entity loadModel(const char* directory)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(directory, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	assert(("[ERROR] Assimp failed to load model", scene && scene->mRootNode && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)));
	
	std::string directoryString(directory);
	Entity root = processNode(scene->mRootNode, scene, directoryString.substr(0, directoryString.find_last_of("/")));
	if (directoryString.find("fbx") != std::string::npos)
	{
		root.getComponent<Transform>().scale *= 0.01f;
		unsigned int nameStart = directoryString.find_last_of("/") + 1;
		const Entity newRoot(directoryString.substr(nameStart, directoryString.find_last_of(".") - nameStart));
		Transform& newRootTransform = newRoot.addComponent<Transform>(TransformCreateInfo{});
		newRootTransform.addChild(root);
		return newRoot;
	}

	return root;
}

void applyModelMaterial(const Entity& model, const Material& material)
{
	std::vector<Mesh*> meshes = getComponentsInHierarchy3D<Mesh>(model);
	for (Mesh* mesh : meshes)
	{
		mesh->material = material;
		mesh->updateMaterial();
	}
}
