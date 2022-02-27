#pragma once
#include "Texture.h"
#include "Camera.h"
#include "WindowManager.h"

#define VSYNC true

#define IRRADIANCE_WIDTH_HEIGHT 64
#define PREFILTER_WIDTH_HEIGHT 720

#define MAX_DIRECTIONAL_LIGHTS 500
#define MAX_POINT_LIGHTS 500
#define MAX_SPOT_LIGHTS 500

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

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexMemory;
	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingMemory;
	Vertex* vertices;

	VkBuffer indexBuffer;
	VkDeviceMemory indexMemory;
	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingMemory;
	unsigned int* indices;

	VkBuffer uniformBuffer; // Stores model and normal matrices.
	VkDeviceMemory uniformMemory;
	VkBuffer uniformStagingBuffer;
	VkDeviceMemory uniformStagingMemory;
	unsigned char* uniformData;

	VkDescriptorSet descriptorSet;

	/*
	Reallocates GPU and CPU side buffers to accommodate [nVertices] vertices and [nIndices] indices. Useful if you wish to change the number of vertices and/or indices.
	Note: All data currently stored in the buffers is wiped.
	*/
	void reallocateBuffers();

	/*
	Copies CPU side buffers to GPU side buffers. Call this procedure to make changes to the vertices or indices take effect.
	*/
	void updateBuffers();

	/*
	Updates information regarding which textures to use for the mesh. Call this procedure to make changes to the material take effect.
	*/
	void updateMaterial();

	/*
	\return An array of positions for each vertex in the mesh.
	*/
	std::vector<glm::vec3> positions();
};

template <>
std::vector<char> serialize(const Mesh& mesh);

template <>
void deserialize(const std::vector<char>& vecData, Mesh& write);

struct DirectionalLight
{
	VkBuffer uniformBuffer; // Stores colour and direction.
	VkDeviceMemory uniformMemory;
	VkBuffer uniformStagingBuffer;
	VkDeviceMemory uniformStagingMemory;
	unsigned char* uniformData;

	VkDescriptorSet descriptorSet;

	glm::vec3 lastColour;
	glm::vec3 colour;
};

struct DirectionalLightCreateInfo
{
	glm::vec3 colour;

	operator DirectionalLight() const;
};

struct UIText
{
	glm::vec2 position;
	float scale;

	std::string text;
	std::string font;
	glm::vec3 colour;
};

typedef std::function<void()> ButtonCallback;

struct UIButton
{
	bool _pressed;

	glm::vec2 position;
	glm::vec2 extent;

	glm::vec3 colour;

	ButtonCallback callback;

	Texture* _unpressedTexture;
	Texture* _canpressTexture;
	Texture* _pressedTexture;

	VkDescriptorSet _unpressedDescriptorSet;
	VkDescriptorSet _canpressDescriptorSet;
	VkDescriptorSet _pressedDescriptorSet;
};

struct UIButtonCreateInfo
{
	glm::vec2 position;
	glm::vec2 extent;

	glm::vec3 colour;

	std::string unpressed;
	std::string canpress;
	std::string pressed;

	ButtonCallback callback;

	operator UIButton() const;
};

class RenderSystem
{
private:
	friend Mesh;

	friend class TextureManager;
	friend class Texture;
	friend class Cubemap;
	friend class FontManager;

	WindowManager& mWindowManager = WindowManager::instance();

	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Mesh>& mMeshManager = ComponentManager<Mesh>::instance();
	ComponentManager<DirectionalLight>& mDirectionalLightManager = ComponentManager<DirectionalLight>::instance();
	ComponentManager<UIText>& mUITextManager = ComponentManager<UIText>::instance();
	ComponentManager<UIButton>& mUIButtonManager = ComponentManager<UIButton>::instance();

	const ComponentAddedCallback mTransformAddedCallback = std::bind(&RenderSystem::transformAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mTransformRemovedCallback = std::bind(&RenderSystem::transformRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mMeshAddedCallback = std::bind(&RenderSystem::meshAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mMeshRemovedCallback = std::bind(&RenderSystem::meshRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mDirectionalLightAddedCallback = std::bind(&RenderSystem::directionalLightAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mDirectionalLightRemovedCallback = std::bind(&RenderSystem::directionalLightRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mUITextAddedCallback = std::bind(&RenderSystem::UITextAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mUITextRemovedCallback = std::bind(&RenderSystem::UITextRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mUIButtonAddedCallback = std::bind(&RenderSystem::UIButtonAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mUIButtonRemovedCallback = std::bind(&RenderSystem::UIButtonRemoved, this, std::placeholders::_1);

	const TransformChangedCallback mMeshTransformChangedCallback = std::bind(&RenderSystem::meshTransformChanged, this, std::placeholders::_1);
	const TransformChangedCallback mDirectionalLightTransformChangedCallback = std::bind(&RenderSystem::directionalLightTransformChanged, this, std::placeholders::_1);
	const std::function<void(const Camera&)> mProjectionChangedCallback = std::bind(&RenderSystem::cameraProjectionChanged, this, std::placeholders::_1);
	const std::function<void(const Transform&, const Camera&)> mViewChangedCallback = std::bind(&RenderSystem::cameraViewChanged, this, std::placeholders::_1, std::placeholders::_2);

	Composition mMeshComposition;
	Composition mDirectionalLightComposition;

	std::vector<EntityID> mMeshIDs;
	std::vector<EntityID> mDirectionalLightIDs;
	std::vector<EntityID> mUITextIDs;
	std::vector<EntityID> mUIButtonIDs;

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

	VkRenderPass mMainRenderPass = VK_NULL_HANDLE;
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

	// mUITextPipeline
	VkDescriptorSetLayout mImageDescriptorSetLayout = {};
	/* ---------------------- */
	
	VkDescriptorSet mCameraDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet mSkyboxDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet mEnvironmentDescriptorSet = VK_NULL_HANDLE;

	// PBR
	VkShaderModule mVertexShader = VK_NULL_HANDLE;
	VkShaderModule mFragmentShader = VK_NULL_HANDLE;

	// Skybox
	VkShaderModule mSkyboxVertexShader = VK_NULL_HANDLE;
	VkShaderModule mSkyboxFragmentShader = VK_NULL_HANDLE;

	// Image Based Lighting
	VkShaderModule mCubemapVertexShader = VK_NULL_HANDLE;
	VkShaderModule mCubemapGeometryShader = VK_NULL_HANDLE;
	VkShaderModule mConvoluteFragmentShader = VK_NULL_HANDLE;
	VkShaderModule mPrefilterFragmentShader = VK_NULL_HANDLE;

	// UI
	VkShaderModule mQuadVertexShader = VK_NULL_HANDLE;
	VkShaderModule mUIFragmentShader = VK_NULL_HANDLE;

	VkPipelineLayout mDirectionalPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mSkyboxPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mConvolutePipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mPrefilterPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mUIPipelineLayout = VK_NULL_HANDLE;

	VkPipeline mDirectionalPipeline = VK_NULL_HANDLE;
	VkPipeline mSkyboxPipeline = VK_NULL_HANDLE;
	VkPipeline mConvolutePipeline = VK_NULL_HANDLE;
	VkPipeline mPrefilterPipeline = VK_NULL_HANDLE;
	VkPipeline mUIPipeline = VK_NULL_HANDLE;

	VkViewport mPrefilterViewport = {};
	VkRect2D mPrefilterScissor = {};

	VkCommandPoolCreateInfo mGraphicsCommandPoolCreateInfo;
	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo mCommandBufferBeginInfo = {};

	VkBuffer mCubeVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mCubeVertexMemory = VK_NULL_HANDLE;

	VkBuffer mUIQuadVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mUIQuadVertexMemory = VK_NULL_HANDLE;

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

	void UITextAdded(const Entity& entity);
	void UITextRemoved(const Entity& entity);
	void UIButtonAdded(const Entity& entity);
	void UIButtonRemoved(const Entity& entity);

	void meshTransformChanged(Transform& transform) const;
	void directionalLightTransformChanged(Transform& transform) const;

	void directionalLightChanged(const DirectionalLight& directionalLight);

	void cameraProjectionChanged(const Camera& camera);
	void cameraViewChanged(const Transform& transform, const Camera& camera);

	void update();

	void setCamera(const Entity& entity);
	void setSkybox(const Cubemap* cubemap);
};

Entity loadModel(const char* directory);

template <typename T>
std::vector<T*> getComponentsInHierarchy(const Entity& entity)
{
	std::vector<T*> components;
	if (entity.hasComponent<T>())
	{
		components.push_back(&entity.getComponent<T>());
	}
	
	Transform& transform = entity.getComponent<Transform>();
	for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
	{
		std::vector<T*> childComponents = getComponentsInHierarchy<T>(Entity(transform.childrenIDs[i]));
		components.insert(components.end(), childComponents.begin(), childComponents.end());
	}

	return components;
}

void applyModelMaterial(const Entity& model, const Material& material);