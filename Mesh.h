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

// PBR Material
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
	// Number of vertices and indices in the vertex and index buffers, can be assigned to but buffers will not be reallocated until reallocateBuffers() is called
	unsigned int nVertices;
	unsigned int nIndices;

	Material material;

	// Vertex buffer
	VkBuffer _vertexBuffer;
	VkDeviceMemory _vertexMemory;
	VkBuffer _vertexStagingBuffer;
	VkDeviceMemory _vertexStagingMemory;

	// Index buffer
	VkBuffer _indexBuffer;
	VkDeviceMemory _indexMemory;
	VkBuffer _indexStagingBuffer;
	VkDeviceMemory _indexStagingMemory;

	// CPU side buffers storing vertices and indices, elements can be overwritten and assigned to, but changes will not take effect until updateBuffers() is called
	Vertex* vertices;
	unsigned int* indices;

	// Uniform buffer stores model and normal matrices
	VkBuffer _uniformBuffer; 
	VkDeviceMemory _uniformMemory;
	VkBuffer _uniformStagingBuffer;
	VkDeviceMemory _uniformStagingMemory;
	unsigned char* _uniformData;

	// Descriptor set references uniform data i.e uniform buffer & material textures for shader to use
	VkDescriptorSet _descriptorSet;

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
	Updates which textures are used by the mesh. Call this procedure to make changes to the material take effect.
	*/
	void updateMaterial();

	/*
	\return An array of positions for each vertex in the mesh.
	*/
	std::vector<glm::vec3> positions() const;
};

template <>
std::vector<char> serialize(const Mesh& mesh);

template <>
void deserialize(const std::vector<char>& vecData, Mesh& write);

struct DirectionalLight
{
	// Uniform buffer stores colour and direction.
	VkBuffer _uniformBuffer;
	VkDeviceMemory _uniformMemory;
	VkBuffer _uniformStagingBuffer;
	VkDeviceMemory _uniformStagingMemory;
	unsigned char* _uniformData;

	VkDescriptorSet _descriptorSet;

	// Store last frames state, allows render system to detect change and update the uniform buffer
	glm::vec3 _lastColour;

	// Can be freely assigned to in order to change the light's colour
	glm::vec3 colour;
};

struct DirectionalLightCreateInfo
{
	glm::vec3 colour;

	// 'Constructs' DirectionalLight
	operator DirectionalLight() const;
};

struct Sprite
{
	unsigned int width;
	unsigned int height;

	Texture* texture;

	VkDescriptorSet _descriptorSet;
};

struct SpriteCreateInfo
{
	unsigned int width;
	unsigned int height;

	std::string texture;

	// 'Constructs' Sprite
	operator Sprite();
};

struct UIText
{
	std::string text;
	std::string font;
	glm::vec3 colour;
};

typedef std::function<void()> ButtonCallback;

struct UIButton
{
	unsigned int width;
	unsigned int height;
	glm::vec3 colour;
	ButtonCallback callback;
	bool toggle;

	Texture* unpressedTexture;
	Texture* canpressTexture;
	Texture* pressedTexture;

	VkDescriptorSet _unpressedDescriptorSet;
	VkDescriptorSet _canpressDescriptorSet;
	VkDescriptorSet _pressedDescriptorSet;

	bool _pressed;
	bool _underCursor;
};

struct UIButtonCreateInfo
{
	unsigned int width;
	unsigned int height;

	std::string unpressed;
	std::string canpress;
	std::string pressed;

	glm::vec3 colour;

	ButtonCallback callback;

	bool toggle = false;

	// 'Constructs' Button
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
	ComponentManager<Transform2D>& mTransform2DManager = ComponentManager<Transform2D>::instance();
	ComponentManager<Sprite>& mSpriteManager = ComponentManager<Sprite>::instance();
	ComponentManager<UIText>& mUITextManager = ComponentManager<UIText>::instance();
	ComponentManager<UIButton>& mUIButtonManager = ComponentManager<UIButton>::instance();

	// Callbacks called externally to ensure the render system is up to date
	const ComponentAddedCallback mTransformAddedCallback = std::bind(&RenderSystem::transformAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mTransformRemovedCallback = std::bind(&RenderSystem::transformRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mMeshAddedCallback = std::bind(&RenderSystem::meshAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mMeshRemovedCallback = std::bind(&RenderSystem::meshRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mDirectionalLightAddedCallback = std::bind(&RenderSystem::directionalLightAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mDirectionalLightRemovedCallback = std::bind(&RenderSystem::directionalLightRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mTransform2DAddedCallback = std::bind(&RenderSystem::transform2DAdded, this, std::placeholders::_1);
	const ComponentAddedCallback mTransform2DRemovedCallback = std::bind(&RenderSystem::transform2DRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mSpriteAddedCallback = std::bind(&RenderSystem::spriteAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mSpriteRemovedCallback = std::bind(&RenderSystem::spriteRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mUITextAddedCallback = std::bind(&RenderSystem::UITextAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mUITextRemovedCallback = std::bind(&RenderSystem::UITextRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mUIButtonAddedCallback = std::bind(&RenderSystem::UIButtonAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mUIButtonRemovedCallback = std::bind(&RenderSystem::UIButtonRemoved, this, std::placeholders::_1);

	const TransformChangedCallback mMeshTransformChangedCallback = std::bind(&RenderSystem::meshTransformChanged, this, std::placeholders::_1);
	const TransformChangedCallback mDirectionalLightTransformChangedCallback = std::bind(&RenderSystem::directionalLightTransformChanged, this, std::placeholders::_1);
	const std::function<void(const Camera&)> mProjectionChangedCallback = std::bind(&RenderSystem::cameraProjectionChanged, this, std::placeholders::_1);
	const std::function<void(const Transform&, const Camera&)> mViewChangedCallback = std::bind(&RenderSystem::cameraViewChanged, this, std::placeholders::_1, std::placeholders::_2);

	const MouseButtonCallback mLMBPressedCallback = std::bind(&RenderSystem::LMBPressed, this);

	// Compositions of entities that are included in the render system
	Composition mMeshComposition;
	Composition mDirectionalLightComposition;

	Composition mSpriteComposition;
	Composition mUITextComposition;
	Composition mUIButtonComposition;

	// Dynamic arrays containing entities included in the render system
	std::vector<EntityID> mMeshIDs;
	std::vector<EntityID> mDirectionalLightIDs;
	std::vector<EntityID> mSpriteIDs;
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
	VkShaderModule m2DFragmentShader = VK_NULL_HANDLE;

	VkPipelineLayout mDirectionalPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mSkyboxPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mConvolutePipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout mPrefilterPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout m2DPipelineLayout = VK_NULL_HANDLE;

	VkPipeline mDirectionalPipeline = VK_NULL_HANDLE;
	VkPipeline mSkyboxPipeline = VK_NULL_HANDLE;
	VkPipeline mConvolutePipeline = VK_NULL_HANDLE;
	VkPipeline mPrefilterPipeline = VK_NULL_HANDLE;
	VkPipeline m2DPipeline = VK_NULL_HANDLE;

	VkViewport mPrefilterViewport = {};
	VkRect2D mPrefilterScissor = {};

	VkCommandPoolCreateInfo mGraphicsCommandPoolCreateInfo;
	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo mCommandBufferBeginInfo = {};

	VkBuffer mCubeVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mCubeVertexMemory = VK_NULL_HANDLE;

	VkBuffer mQuadVertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory mQuadVertexMemory = VK_NULL_HANDLE;

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

	// Used to stretch 2D images by aspect ratio so they are displayed correctly
	glm::mat4 m2DProjection;

	RenderSystem();
public:
	void start();

private:
	void allocateMeshBuffers(Mesh& mesh);
	void addMesh(const Entity& entity);
	void removeMesh(const std::vector<EntityID>::iterator& IDIterator);

	void addDirectionalLight(const Entity& entity);
	void removeDirectionalLight(const std::vector<EntityID>::iterator& IDIterator);

	void addSprite(const Entity& entity);
	void removeSprite(const std::vector<EntityID>::iterator& IDIterator);

	void addUIButton(const Entity& entity);
	void removeUIButton(const std::vector<EntityID>::iterator& IDIterator);

public:
	static RenderSystem& instance();

	RenderSystem(const RenderSystem& copy) = delete;
	~RenderSystem();

	// Callback subroutines. Should not be used outside of their internal use
	void transformAdded(const Entity& entity);
	void transformRemoved(const Entity& entity);
	void meshAdded(const Entity& entity);
	void meshRemoved(const Entity& entity);
	void directionalLightAdded(const Entity& entity);
	void directionalLightRemoved(const Entity& entity);

	void transform2DAdded(const Entity& entity);
	void transform2DRemoved(const Entity& entity);
	void spriteAdded(const Entity& entity);
	void spriteRemoved(const Entity& entity);
	void UITextAdded(const Entity& entity);
	void UITextRemoved(const Entity& entity);
	void UIButtonAdded(const Entity& entity);
	void UIButtonRemoved(const Entity& entity);

	void meshTransformChanged(Transform& transform) const;
	void directionalLightTransformChanged(Transform& transform) const;

	void directionalLightChanged(const DirectionalLight& directionalLight);

	void cameraProjectionChanged(const Camera& camera);
	void cameraViewChanged(const Transform& transform, const Camera& camera);

	void LMBPressed();

	// Renders scene
	void update();

	/*
	Sets the active camera.
	\param entity: Entity to make the active camera. Must possess a Transform and Camera component.
	*/
	void setCamera(const Entity& entity);

	/*
	Sets the active skybox.
	\param cubemap: Cubemap to use as the skybox. Used to calculate IBL so HDR cubemaps provide better results.
	*/
	void setSkybox(const Cubemap* cubemap);
};

/*
Load model from file e.g obj, fbx, etc.
\param directory: Directory of the file to load.
\return A root parent entity of the model. Use transforms to traverse the model. 
*/
Entity loadModel(const char* directory);

/*
Retrieves an array of a specific component from the entity and its children.
\param entity: The parent entity to get components from.
\return A dynamic array containing pointers to each component.
*/
template <typename T>
std::vector<T*> getComponentsInHierarchy3D(const Entity& entity)
{
	std::vector<T*> components;
	if (entity.hasComponent<T>())
		components.push_back(&entity.getComponent<T>());

	Transform& transform = entity.getComponent<Transform>();
	for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
	{
		std::vector<T*> childComponents = getComponentsInHierarchy3D<T>(Entity(transform.childrenIDs[i]));
		components.insert(components.end(), childComponents.begin(), childComponents.end());
	}

	return components;
}

/*
Retrieves an array of a specific component from the entity and its children.
\param entity: The parent entity to get components from.
\return A dynamic array containing pointers to each component.
*/
template <typename T>
std::vector<T*> getComponentsInHierarchy2D(const Entity& entity)
{
	std::vector<T*> components;
	if (entity.hasComponent<T>())
		components.push_back(&entity.getComponent<T>());

	Transform2D& transform = entity.getComponent<Transform2D>();
	for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
	{
		std::vector<T*> childComponents = getComponentsInHierarchy2D<T>(Entity(transform.childrenIDs[i]));
		components.insert(components.end(), childComponents.begin(), childComponents.end());
	}

	return components;
}

/*
Applys a material to an entity and all its children.
\param model: Parent of entities to apply material to.
\param material: Material to apply.
*/
void applyModelMaterial(const Entity& model, const Material& material);