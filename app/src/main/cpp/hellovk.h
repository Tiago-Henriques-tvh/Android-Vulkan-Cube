#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <assert.h>
#include <vulkan/vulkan.h>

#include <array>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vkt {
#define LOG_TAG "hellovkjni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define VK_CHECK(x)                           \
  do {                                        \
    VkResult err = x;                         \
    if (err) {                                \
      LOGE("Detected Vulkan error: %d", err); \
      abort();                                \
    }                                         \
  } while (0)

    /*
     * Each GPU has several families of queues that process different types of commands. Queue Types:
     * Graphics (GRAPHICS): Processes graphics commands such as drawing and rendering.
     * Compute (COMPUTE): Processes parallel calculations, such as scientific computing.
     * Transfer (TRANSFER): Handles memory transfers.
     */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    /*
     * VkSurfaceKHR, Represents a surface for rendering, associated with the application window.
     * Required to display graphics on the screen. The application creates a surface from an existing
     * window.
     *
     * VkSwapchainKHR: infrastructure that contains images that will be displayed on the screen.
     * 1. The application acquires an image of the exchange chain.
     * 2. Draw on the image.
     * 3. Display the image back on the screen.
     */
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct ANativeWindowDeleter {
        void operator()(ANativeWindow *window) { ANativeWindow_release(window); }
    };

    // for double buffering
    const int MAX_FRAMES_IN_FLIGHT = 2;
    // separate descriptor sets for the cube, plane, texture and light for each frame
    const int DESCRIPTOR_SETS_PER_FRAME = 4;

    struct DrawObject {
        uint32_t indexCount;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        VkDescriptorSet descriptorSet;
        std::optional<VkDescriptorSet> textureDescriptorSet;  // Use std::optional
        uint32_t firstIndex;
    };

    struct LightUBO {
        glm::vec3 position;   // For point light (not needed for directional)
        glm::vec3 direction;  // For directional light (only needed for directional)
        glm::vec3 color;      // LightUBO color (typically RGB)
        float intensity;      // LightUBO intensity
        float constant;       // Point light attenuation (constant)
        float linear;         // Point light attenuation (linear)
        float quadratic;      // Point light attenuation (quadratic)
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 textCoord;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, textCoord);

            return attributeDescriptions;
        }
    };

    const std::vector<Vertex> cubeVertices = {
            // Front face (light pink)
            {{-0.5f, -0.5f, 0.5f},  {0.9f, 0.7f,  0.8f},  {-1.0f, -1.0f}},
            {{-0.5f, 0.5f,  0.5f},  {0.9f, 0.7f,  0.8f},  {-1.0f, -1.0f}},
            {{0.5f,  0.5f,  0.5f},  {0.9f, 0.7f,  0.8f},  {-1.0f, -1.0f}},
            {{0.5f,  -0.5f, 0.5f},  {0.9f, 0.7f,  0.8f},  {-1.0f, -1.0f}},

            // Back face (light green)
            {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.9f,  0.7f},  {-1.0f, -1.0f}},
            {{-0.5f, 0.5f,  -0.5f}, {0.7f, 0.9f,  0.7f},  {-1.0f, -1.0f}},
            {{0.5f,  0.5f,  -0.5f}, {0.7f, 0.9f,  0.7f},  {-1.0f, -1.0f}},
            {{0.5f,  -0.5f, -0.5f}, {0.7f, 0.9f,  0.7f},  {-1.0f, -1.0f}},

            // Left face (light blue)
            {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.8f,  0.9f},  {-1.0f, -1.0f}},
            {{-0.5f, -0.5f, 0.5f},  {0.7f, 0.8f,  0.9f},  {-1.0f, -1.0f}},
            {{-0.5f, 0.5f,  0.5f},  {0.7f, 0.8f,  0.9f},  {-1.0f, -1.0f}},
            {{-0.5f, 0.5f,  -0.5f}, {0.7f, 0.8f,  0.9f},  {-1.0f, -1.0f}},

            // Right face (light yellow)
            {{0.5f,  -0.5f, -0.5f}, {0.9f, 0.9f,  0.6f},  {-1.0f, -1.0f}},
            {{0.5f,  0.5f,  -0.5f}, {0.9f, 0.9f,  0.6f},  {-1.0f, -1.0f}},
            {{0.5f,  0.5f,  0.5f},  {0.9f, 0.9f,  0.6f},  {-1.0f, -1.0f}},
            {{0.5f,  -0.5f, 0.5f},  {0.9f, 0.9f,  0.6f},  {-1.0f, -1.0f}},

            // Top face (light lavender)
            {{-0.5f, 0.5f,  -0.5f}, {0.8f, 0.7f,  0.9f},  {-1.0f, -1.0f}},
            {{-0.5f, 0.5f,  0.5f},  {0.8f, 0.7f,  0.9f},  {-1.0f, -1.0f}},
            {{0.5f,  0.5f,  0.5f},  {0.8f, 0.7f,  0.9f},  {-1.0f, -1.0f}},
            {{0.5f,  0.5f,  -0.5f}, {0.8f, 0.7f,  0.9f},  {-1.0f, -1.0f}},

            // Bottom face (light peach)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.85f, 0.75f}, {-1.0f, -1.0f}},
            {{0.5f,  -0.5f, -0.5f}, {1.0f, 0.85f, 0.75f}, {-1.0f, -1.0f}},
            {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.85f, 0.75f}, {-1.0f, -1.0f}},
            {{-0.5f, -0.5f, 0.5f},  {1.0f, 0.85f, 0.75f}, {-1.0f, -1.0f}},
    };

    const std::vector<uint16_t> cubeIndices = {
            0, 3, 2, 2, 1, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
    };

    const std::vector<Vertex> planeVertices = {
            // Front face
            {{-1.2f, -0.1f, 1.2f},  {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{-1.2f, 0.1f,  1.2f},  {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{1.2f,  0.1f,  1.2f},  {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{1.2f,  -0.1f, 1.2f},  {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},

            // Back face
            {{-1.2f, -0.1f, -1.2f}, {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{-1.2f, 0.1f,  -1.2f}, {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{1.2f,  0.1f,  -1.2f}, {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{1.2f,  -0.1f, -1.2f}, {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},

            // Left face
            {{-1.2f, -0.1f, -1.2f}, {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{-1.2f, -0.1f, 1.2f},  {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{-1.2f, 0.1f,  1.2f},  {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{-1.2f, 0.1f,  -1.2f}, {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},

            // Right face
            {{1.2f,  -0.1f, -1.2f}, {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{1.2f,  0.1f,  -1.2f}, {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{1.2f,  0.1f,  1.2f},  {0.4f, 0.4f, 0.4f}, {-1.0f, -1.0f}},
            {{1.2f,  -0.1f, 1.2f},  {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},

            // Top face
            {{-1.2f, 0.1f,  -1.2f}, {0.4f, 0.4f, 0.4f}, {0.0f,  0.0f}},
            {{-1.2f, 0.1f,  1.2f},  {0.4f, 0.4f, 0.4f}, {0.0f,  1.0f}},
            {{1.2f,  0.1f,  1.2f},  {0.4f, 0.4f, 0.4f}, {1.0f,  1.0f}},
            {{1.2f,  0.1f,  -1.2f}, {0.4f, 0.4f, 0.4f}, {1.0f,  0.0f}},

            // Bottom face
            {{-1.2f, -0.1f, -1.2f}, {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{1.2f,  -0.1f, -1.2f}, {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{1.2f,  -0.1f, 1.2f},  {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
            {{-1.2f, -0.1f, 1.2f},  {0.2f, 0.2f, 0.2f}, {-1.0f, -1.0f}},
    };

    const std::vector<uint16_t> planeIndices = {
            0, 3, 2, 2, 1, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
    };

    class HelloVK {
    public:
        void initVulkan();

        void render();

        void cleanup();

        void cleanupSwapChain();

        void reset(ANativeWindow *newWindow, AAssetManager *newManager);

        bool initialized = false;

    private:
        void createInstance();

        void createSurface();

        void setupDebugMessenger();

        void pickPhysicalDevice();

        void createLogicalDeviceAndQueue();

        void createSwapChain();

        void createImageViews();

        void createRenderPass();

        void createDescriptorSetLayouts();

        void createGraphicsPipeline();

        void createFramebuffers();

        void createCommandPool();

        void createCommandBuffers();

        void createSyncObjects();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        bool isDeviceSuitable(VkPhysicalDevice device);

        bool checkValidationLayerSupport();

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

        VkShaderModule createShaderModule(const std::vector<uint8_t> &code);

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void recreateSwapChain();

        VkCommandBuffer beginSingleTimeCommands();

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory);

        void createUniformBuffers();

        void updateUniformBuffer(uint32_t currentImage);

        void createDescriptorPool();

        void createDescriptorSets();

        void establishDisplaySizeIdentity();

        void createIndexBuffer();

        void createVertexBuffer();

        void updateCubeUniformBuffer(glm::mat4 model, glm::mat4 view, glm::mat4 proj,
                                     uint32_t currentImage);

        void updatePlaneUniformBuffer(glm::mat4 model, glm::mat4 view, glm::mat4 proj,
                                      uint32_t currentImage);

        void updateLightBuffer(uint32_t currentImage);

        void decodeImage();

        void copyBufferToImage();

        void createTextureImageViews();

        void createTextureImage();

        void createTextureSampler();

        // Native window and asset manager
        std::unique_ptr<ANativeWindow, ANativeWindowDeleter> window; // Android native window
        AAssetManager *assetManager;                                // Android asset manager

        // Vulkan instance and debug utilities
        VkInstance instance;                                        // Vulkan instance
        VkDebugUtilsMessengerEXT debugMessenger;                    // Debug messenger for validation layers

        // Surface and physical device
        VkSurfaceKHR surface;                                       // Surface for presenting
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;           // Selected physical device (GPU)

        // Logical device and queues
        VkDevice device;                                            // Logical device
        VkQueue graphicsQueue;                                      // Queue for graphics commands
        VkQueue presentQueue;                                       // Queue for presenting commands

        // Swapchain and related objects
        VkSwapchainKHR swapChain;                                   // Swapchain for presenting images
        std::vector<VkImage> swapChainImages;                       // Images in the swapchain
        VkFormat swapChainImageFormat;                              // Format of swapchain images
        VkExtent2D swapChainExtent;                                 // Extent (resolution) of the swapchain
        VkExtent2D displaySizeIdentity;                             // Identity resolution for display scaling
        std::vector<VkImageView> swapChainImageViews;               // Image views for swapchain images
        std::vector<VkFramebuffer> swapChainFramebuffers;           // Framebuffers for rendering to the swapchain

        // Command buffers and command pool
        VkCommandPool commandPool;                                  // Command pool for allocating command buffers
        std::vector<VkCommandBuffer> commandBuffers;                // Command buffers for recording drawing commands

        // Render pass and pipeline
        VkRenderPass renderPass;                                    // Render pass configuration
        VkDescriptorSetLayout objectDescriptorSetLayout;            // Layout for descriptor sets
        VkDescriptorSetLayout lightDescriptorSetLayout;             // Layout for descriptor sets
        VkDescriptorSetLayout textureDescriptorSetLayout;           // Layout for descriptor sets
        VkPipelineLayout pipelineLayout;                            // Layout for graphics pipeline
        VkPipeline graphicsPipeline;                                // Graphics pipeline

        // Synchronization primitives
        std::vector<VkSemaphore> imageAvailableSemaphores;          // Semaphores for image availability
        std::vector<VkSemaphore> renderFinishedSemaphores;          // Semaphores for rendering completion
        std::vector<VkFence> inFlightFences;                        // Fences for GPU-CPU synchronization

        // Uniform buffers
        std::vector<VkBuffer> cubeUniformBuffers;                   // Uniform buffers for the cube
        std::vector<VkDeviceMemory> cubeUniformBuffersMemory;       // Memory for cube uniform buffers
        std::vector<VkBuffer> planeUniformBuffers;                  // Uniform buffers for the plane
        std::vector<VkDeviceMemory> planeUniformBuffersMemory;      // Memory for plane uniform buffers
        std::vector<VkBuffer> lightUniformBuffers;                  // Uniform buffers for the light
        std::vector<VkDeviceMemory> lightUniformBuffersMemory;      // Memory for light uniform buffers

        // Descriptor pool and sets
        VkDescriptorPool descriptorPool;                            // Descriptor pool for allocation
        std::vector<VkDescriptorSet> cubeDescriptorSets;            // Descriptor sets for cube
        std::vector<VkDescriptorSet> planeDescriptorSets;           // Descriptor sets for plane
        std::vector<VkDescriptorSet> lightDescriptorSets;           // Descriptor sets for lights
        std::vector<VkDescriptorSet> textureDescriptorSets;         // Descriptor sets for textures

        // Vertex and index buffers
        VkBuffer vertexBuffer;                                      // Buffer for vertex data
        VkDeviceMemory vertexBufferMemory;                          // Memory for vertex buffer
        VkBuffer indexBuffer;                                       // Buffer for index data
        VkDeviceMemory indexBufferMemory;                           // Memory for index buffer

        // Textures
        VkBuffer imgStagingBuffer;
        VkDeviceMemory imgStagingMemory;
        int textureWidth, textureHeight, textureChannels;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        // Frame tracking and orientation
        uint32_t currentFrame = 0;                                  // Current frame index
        bool orientationChanged = false;                            // Flag for orientation changes
        VkSurfaceTransformFlagBitsKHR pretransformFlag;             // Surface pre-transform flag

        // Validation layers and extensions
        bool enableValidationLayers = true;                        // Enable or disable validation layers
        const std::vector<const char *> validationLayers = {        // Validation layer names
                "VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> deviceExtensions = {        // Device extension names
                VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };

}  // namespace vkt