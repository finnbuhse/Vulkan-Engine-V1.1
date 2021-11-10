#pragma once
/* *NOTE* Camera.h includes the most detailed explanation of a component and system example */

#include "Texture.h"
#include "Vulkan.h"
#include "Camera.h"

struct Material
{
	Texture* albedo;
	Texture* normal;
	Texture* roughness;
	Texture* metalness;
	Texture* ambientOcclusion;
};

struct MaterialCreateInfo
{
	std::string albedo = "";
	std::string normal = "";
	std::string roughness = "";
	std::string metalness = "";
	std::string ambientOcclusion = "";

	operator Material() const;
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 textureCoordinate;
};

struct Mesh
{
	unsigned int nVertices;
	unsigned int nIndices;
	Material material;

	// Vertex buffer stores vertices
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexMemory;
	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingMemory;
	Vertex* vertices;

	// Index buffer stores indices
	VkBuffer indexBuffer;
	VkDeviceMemory indexMemory;
	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingMemory;
	unsigned int* indices;

	// Uniform buffer stores model and normal matrix
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;
	VkBuffer uniformStagingBuffer;
	VkDeviceMemory uniformStagingMemory;
	unsigned char* uniformData;

	VkDescriptorSet descriptorSet;

	unsigned int transformChangedCallbackIndex;

	void reallocateBuffers(); // Should be called if nVertices or nIndices are changed
	void updateBuffers(); // Should be called if vertices or indices are changed

	void updateMaterial(); // Should be called if the material is changed

	std::vector<glm::vec3> positions();

	Mesh serializeInfo() const { return *this; } // Specific serialize/deserialize method used
};

template <>
std::vector<char> serialize(const Mesh& mesh);

template <>
void deserialize(const std::vector<char>& vecData, Mesh& write);

struct DirectionalLight;

struct DirectionalLightCreateInfo
{
	glm::vec3 colour;

	operator DirectionalLight() const;
};

struct DirectionalLight
{
	// Uniform buffer stores colour and direction
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformMemory;
	VkBuffer uniformStagingBuffer;
	VkDeviceMemory uniformStagingMemory;
	unsigned char* uniformData;

	VkDescriptorSet descriptorSet;

	glm::vec3 lastColour;
	glm::vec3 colour;

	unsigned int transformChangedCallbackIndex;

	DirectionalLightCreateInfo serializeInfo() const;
};

class RenderSystem
{
private:
	friend Mesh;

	friend class TextureManager;
	friend class Texture;
	friend class Cubemap;

	Composition mMeshComposition;
	Composition mDirectionalLightComposition;

	std::vector<EntityID> mMeshIDs;
	std::vector<EntityID> mDirectionalLightIDs;

	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Mesh>& mMeshManager = ComponentManager<Mesh>::instance();
	ComponentManager<DirectionalLight>& mDirectionalLightManager = ComponentManager<DirectionalLight>::instance();
	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();

	#pragma region Vulkan resources
	VkInstance mVkInstance = VK_NULL_HANDLE;
	VkSurfaceKHR mSurface = VK_NULL_HANDLE;
	VkPhysicalDevice* mPhysicalDevices = nullptr;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties mPhysicalDeviceProperties = {};
	VkPhysicalDeviceFeatures mPhysicalDeviceFeatures = {};
	VkPhysicalDeviceMemoryProperties mPhysicalDeviceMemoryProperties = {};
	VkDevice mDevice = VK_NULL_HANDLE;
	VkQueue mGraphicsQueue = VK_NULL_HANDLE;
	VkQueue mPresentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
	VkImageView* mSwapchainImageViews = nullptr;

	VkImage mDepthImage = VK_NULL_HANDLE;
	VkDeviceMemory mDepthMemory = VK_NULL_HANDLE;
	VkImageView mDepthImageView = VK_NULL_HANDLE;

	VkImage mIrradianceImage = VK_NULL_HANDLE;
	VkDeviceMemory mIrradianceMemory = VK_NULL_HANDLE;
	VkImageView mIrradianceImageView = VK_NULL_HANDLE;
	VkSampler mIrradianceSampler = VK_NULL_HANDLE;

	VkImage mPrefilterImage = VK_NULL_HANDLE;
	VkDeviceMemory mPrefilterMemory = VK_NULL_HANDLE;
	VkImageView* mPrefilterImageViews = nullptr;
	VkImageView mPrefilterImageView = VK_NULL_HANDLE;
	VkSampler mPrefilterSampler = VK_NULL_HANDLE;

	VkRenderPass mRenderPass = VK_NULL_HANDLE;
	VkClearValue mClearColours[2] = {};
	VkRenderPassBeginInfo mRenderPassBeginInfo = {};

	VkRenderPass mEnvironmentRenderPass = VK_NULL_HANDLE;
	VkClearValue mEnvironmentClearColour = {};
	VkRenderPassBeginInfo mConvoluteRenderPassBeginInfo = {};
	VkRenderPassBeginInfo mPrefilterRenderPassBeginInfo = {};

	VkFramebuffer* mFramebuffers = nullptr;
	VkFramebuffer mConvoluteFramebuffer = VK_NULL_HANDLE;
	VkFramebuffer* mPrefilterFramebuffers = nullptr;

	VkBuffer mCameraUniformBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mCameraUniformMemory = VK_NULL_HANDLE;
	unsigned char* mUniformData = nullptr;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	
	/* DESCRIPTOR SET LAYOUTS */

	// mDirectionalPipeline
	VkDescriptorSetLayout mDirectionalLightingDescriptorSetLayouts[3] = {};

	// mSkyboxPipeline
	VkDescriptorSetLayout mSkyboxDescriptorSetLayout = VK_NULL_HANDLE;
	
	// mConvolutePipeline
	VkDescriptorSetLayout mEnvironmentDescriptorSetLayout = VK_NULL_HANDLE;
	/* ---------------------- */
	
	VkDescriptorSet mCameraDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet mSkyboxDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet mEnvironmentDescriptorSet = VK_NULL_HANDLE;

	VkShaderModule mVertexShader = VK_NULL_HANDLE;
	VkShaderModule mFragmentShader = VK_NULL_HANDLE;

	VkShaderModule mSkyboxVertexShader = VK_NULL_HANDLE;
	VkShaderModule mSkyboxFragmentShader = VK_NULL_HANDLE;

	VkShaderModule mCubemapVertexShader = VK_NULL_HANDLE;
	VkShaderModule mCubemapGeometryShader = VK_NULL_HANDLE;
	VkShaderModule mConvoluteFragmentShader = VK_NULL_HANDLE;
	VkShaderModule mPrefilterFragmentShader = VK_NULL_HANDLE;

	VkPipelineLayout mDirectionalPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mSkyboxPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mConvolutePipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mPrefilterPipelineLayout = VK_NULL_HANDLE;

	VkPipeline mDirectionalPipeline = VK_NULL_HANDLE;
	VkPipeline mSkyboxPipeline = VK_NULL_HANDLE;

	VkPipeline mConvolutePipeline = VK_NULL_HANDLE;
	VkPipeline mPrefilterPipeline = VK_NULL_HANDLE;
	VkViewport mPrefilterViewport = {};
	VkRect2D mPrefilterScissor = {};

	VkCommandPoolCreateInfo mGraphicsCommandPoolCreateInfo;
	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo mCommandBufferBeginInfo = {};

	VkBuffer mCubeVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mCubeVertexMemory = VK_NULL_HANDLE;

	VkSemaphore mImageAvailable = VK_NULL_HANDLE;
	VkSemaphore mRenderComplete = VK_NULL_HANDLE;
	VkViewport mViewport = {};
	VkRect2D mScissor = {};
	VkPipelineStageFlags mWaitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo mRenderSubmitInfo = {};
	VkPresentInfoKHR mPresentInfo = {};

	unsigned int mGraphicsQueueIndex = UINT32_MAX, mPresentQueueIndex = UINT32_MAX;
	unsigned int mNPhysicalDevices = 0;
	unsigned int mSurfaceWidth = 0;
	unsigned int mSurfaceHeight = 0;
	unsigned int mNSwapchainImages = 0;
	unsigned int mCurrentImage = 0;
	unsigned int mNPrefilterMips = 0;
	#pragma endregion

	EntityID mCamera = NULL;
	unsigned int mCameraProjectionChangedCallbackIndex = 0;
	unsigned int mCameraViewChangedCallbackIndex = 0;

	RenderSystem();
public:
	void initialize();

private:
	void createMeshBuffers(Mesh& mesh);
	void addMesh(const Entity& entity);
	void removeMesh(const std::vector<EntityID>::iterator& IDIterator);

	void addDirectionalLight(const Entity& entity);
	void removeDirectionalLight(const std::vector<EntityID>::iterator& IDIterator);

public:
	static RenderSystem& instance();

	RenderSystem(const RenderSystem& copy) = delete;
	~RenderSystem();

	void transformAdded(const Entity& entity);
	void transformRemoved(const Entity& entity);
	void meshAdded(const Entity& entity);
	void meshRemoved(const Entity& entity);
	void directionalLightAdded(const Entity& entity);
	void directionalLightRemoved(const Entity& entity);

	void meshTransformChanged(Transform& transform);

	void directionalLightChanged(const DirectionalLight& directionalLight);
	void directionalLightTransformChanged(Transform& transform);

	void cameraProjectionChanged(const Camera& camera);
	void cameraViewChanged(const Transform& transform, const Camera& camera);

	void update();

	void setCamera(const Entity& entity);
	void setSkybox(const Cubemap* cubemap);
};

std::vector<Entity> loadModel(const char* directory);

#define destroyModel(model) for (const Entity& entity : model) entity.destroy();

template <typename T>
std::vector<T*> findComponentsInModel(const std::vector<Entity>& model)
{
	std::vector<T*> components;
	for (std::vector<Entity>::const_reverse_iterator it = model.rbegin(); it != model.rend(); it++)
	{
		if ((*it).hasComponent<T>())
			components.push_back(&((*it).getComponent<T>()));
	}
	return components;
}

void applyMaterial(const std::vector<Entity>& model, const Material& material);