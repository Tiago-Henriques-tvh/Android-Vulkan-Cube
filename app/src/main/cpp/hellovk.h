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
        std::optional <uint32_t> graphicsFamily;
        std::optional <uint32_t> presentFamily;

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
        std::vector <VkSurfaceFormatKHR> formats;
        std::vector <VkPresentModeKHR> presentModes;
    };

    struct ANativeWindowDeleter {
        void operator()(ANativeWindow *window) { ANativeWindow_release(window); }
    };

    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct UniformBufferObject {
        glm::mat4 mvp;
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
        void createDevice();
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
        void createCommandBuffer();
        void createSyncObjects();

        // Helper methods
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        bool isDeviceSuitable(VkPhysicalDevice device);

        bool checkValidationLayerSupport();

        std::vector<const char *> getRequiredExtensions(bool enableValidation);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        VkShaderModule createShaderModule(const std::vector <uint8_t> &code);

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void recreateSwapChain();

        void onOrientationChange();

        // Memory and resource management
        uint32_t findMemoryType(uint32_t typeFilter,
                                VkMemoryPropertyFlags properties);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory);

        void createUniformBuffers();

        void updateUniformBuffer(uint32_t currentImage);

        void createDescriptorPool();

        void createDescriptorSets();

        void establishDisplaySizeIdentity();

        std::unique_ptr <ANativeWindow, ANativeWindowDeleter> window;
        AAssetManager *assetManager;

        // Vulkan objects
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        VkSwapchainKHR swapChain;
        std::vector <VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkExtent2D displaySizeIdentity;
        std::vector <VkImageView> swapChainImageViews;
        std::vector <VkFramebuffer> swapChainFramebuffers;
        VkCommandPool commandPool;
        std::vector <VkCommandBuffer> commandBuffers;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        // Synchronization objects
        std::vector <VkBuffer> uniformBuffers;
        std::vector <VkDeviceMemory> uniformBuffersMemory;
        std::vector <VkSemaphore> imageAvailableSemaphores;
        std::vector <VkSemaphore> renderFinishedSemaphores;
        std::vector <VkFence> inFlightFences;
        VkDescriptorPool descriptorPool;
        std::vector <VkDescriptorSet> descriptorSets;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        // Texture management
        int textureWidth, textureHeight, textureChannels;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        // Frame management
        uint32_t currentFrame = 0;
        bool orientationChanged = false;
        VkSurfaceTransformFlagBitsKHR pretransformFlag;

        // Other Vulkan-specific helpers
        /*
         * In order to enable validation layer toggle this to true and
         * follow the README.md instructions concerning the validation
         * layers. You will be required to add separate vulkan validation
         * '*.so' files in order to enable this.
         *
         * The validation layers are not shipped with the APK as they are sizeable.
         */
        bool enableValidationLayers = false;
        const std::vector<const char *> validationLayers = {
                "VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };
}  // namespace vkt