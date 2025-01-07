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
     * Each GPU has several families of queues that process different types of commands.
     *
     * Queue Types:
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

    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            return attributeDescriptions;
        }
    };

    const std::vector<Vertex> cubeVertices = {
            // Front face (light pink)
            {{-0.5f, -0.5f, 0.5f},  {0.9f, 0.7f,  0.8f}},
            {{0.5f,  -0.5f, 0.5f},  {0.9f, 0.7f,  0.8f}},
            {{0.5f,  0.5f,  0.5f},  {0.9f, 0.7f,  0.8f}},
            {{-0.5f, 0.5f,  0.5f},  {0.9f, 0.7f,  0.8f}},

            // Back face (light green)
            {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.9f,  0.7f}},
            {{-0.5f, 0.5f,  -0.5f}, {0.7f, 0.9f,  0.7f}},
            {{0.5f,  0.5f,  -0.5f}, {0.7f, 0.9f,  0.7f}},
            {{0.5f,  -0.5f, -0.5f}, {0.7f, 0.9f,  0.7f}},

            // Left face (light blue)
            {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.8f,  0.9f}},
            {{-0.5f, -0.5f, 0.5f},  {0.7f, 0.8f,  0.9f}},
            {{-0.5f, 0.5f,  0.5f},  {0.7f, 0.8f,  0.9f}},
            {{-0.5f, 0.5f,  -0.5f}, {0.7f, 0.8f,  0.9f}},

            // Right face (light yellow)
            {{0.5f,  -0.5f, -0.5f}, {0.9f, 0.9f,  0.6f}},
            {{0.5f,  0.5f,  -0.5f}, {0.9f, 0.9f,  0.6f}},
            {{0.5f,  0.5f,  0.5f},  {0.9f, 0.9f,  0.6f}},
            {{0.5f,  -0.5f, 0.5f},  {0.9f, 0.9f,  0.6f}},

            // Top face (light lavender)
            {{-0.5f, 0.5f,  -0.5f}, {0.8f, 0.7f,  0.9f}},
            {{-0.5f, 0.5f,  0.5f},  {0.8f, 0.7f,  0.9f}},
            {{0.5f,  0.5f,  0.5f},  {0.8f, 0.7f,  0.9f}},
            {{0.5f,  0.5f,  -0.5f}, {0.8f, 0.7f,  0.9f}},

            // Bottom face (light peach)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.85f, 0.75f}},
            {{0.5f,  -0.5f, -0.5f}, {1.0f, 0.85f, 0.75f}},
            {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.85f, 0.75f}},
            {{-0.5f, -0.5f, 0.5f},  {1.0f, 0.85f, 0.75f}},
    };

    const std::vector<uint16_t> cubeIndices = {
            0, 1, 2, 2, 3, 0, // Front face
            4, 5, 6, 6, 7, 4, // Back face
            8, 9, 10, 10, 11, 8, // Left face
            12, 13, 14, 14, 15, 12, // Right face
            16, 17, 18, 18, 19, 16, // Top face
            20, 21, 22, 22, 23, 20 // Bottom face
    };

    struct TextureVertex {
        glm::vec3 pos;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 1;
            bindingDescription.stride = sizeof(TextureVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            attributeDescriptions[0].binding = 1;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(TextureVertex, pos);

            attributeDescriptions[1].binding = 1;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(TextureVertex, texCoord);

            return attributeDescriptions;
        }
    };

    const std::vector<TextureVertex> planeVertices = {
            {{-1.0f, -1.0f, 1.0f},  {0.0f, 0.0f}}, // Bottom-left
            {{1.0f,  -1.0f, 1.0f},  {1.0f, 0.0f}}, // Bottom-right
            {{1.0f,  -1.0f, -1.0f}, {1.0f, 1.0f}}, // Top-right
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}}  // Top-left
    };

    const std::vector<uint16_t> planeIndices = {
            0, 1, 2, 2, 3, 0
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

        void createTextureImage();

        void decodeImage();

        void createTextureImageViews();

        void createTextureSampler();

        void copyBufferToImage();

        void createRenderPass();

        void createDescriptorSetLayout();

        void createGraphicsPipeline();

        void createFramebuffers();

        void createCommandPool();

        void createCommandBuffers();

        void createSyncObjects();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        bool isDeviceSuitable(VkPhysicalDevice device);

        bool checkValidationLayerSupport();

        static std::vector<const char *> getRequiredExtensions(bool enableValidation);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        VkShaderModule createShaderModule(const std::vector<uint8_t> &code);

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void recreateSwapChain();

        void onOrientationChange();

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

        // Private Variables
        std::unique_ptr<ANativeWindow, ANativeWindowDeleter> window;
        AAssetManager *assetManager;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkExtent2D displaySizeIdentity;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        int textureWidth, textureHeight, textureChannels;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        uint32_t currentFrame = 0;
        bool orientationChanged = false;
        VkSurfaceTransformFlagBitsKHR pretransformFlag;

        // Vulkan-specific
        bool enableValidationLayers = true;
        const std::vector<const char *> validationLayers = {
                "VK_LAYER_KHRONOS_validation"};


        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };

}  // namespace vkt