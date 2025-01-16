#include "hellovk.h"

#define STB_IMAGE_IMPLEMENTATION

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_LEFT_HANDED

#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::vector<uint8_t> LoadBinaryFileToVector(const char *file_path, AAssetManager *assetManager) {
    std::vector<uint8_t> file_content;
    assert(assetManager);
    AAsset *file =
            AAssetManager_open(assetManager, file_path, AASSET_MODE_BUFFER);
    size_t file_length = AAsset_getLength(file);

    file_content.resize(file_length);

    AAsset_read(file, file_content.data(), file_length);
    AAsset_close(file);
    return file_content;
}

const char *toStringMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
    switch (s) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "VERBOSE";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "ERROR";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "INFO";
        default:
            return "UNKNOWN";
    }
}

const char *toStringMessageType(VkDebugUtilsMessageTypeFlagsEXT s) {
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "General | Validation | Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "Validation | Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "General | Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
        return "General | Validation";
    if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) return "Validation";
    if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) return "General";
    return "Unknown";
}

static VKAPI_ATTR VkBool32

VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                         void * /* pUserData */) {
    auto ms = toStringMessageSeverity(messageSeverity);
    auto mt = toStringMessageType(messageType);
    printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

std::vector<const char *> getRequiredExtensions(bool enableValidationLayers) {
    std::vector<const char *> extensions;
    extensions.push_back("VK_KHR_surface");
    extensions.push_back("VK_KHR_android_surface");
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

// -------------------------------------------------------------------------------------------------
// ----------------------------------------- Functions Order ---------------------------------------
// -------------------------------------------------------------------------------------------------

using namespace vkt;

void HelloVK::initVulkan() {
    createInstance();                // Creates the Vulkan instance
    createSurface();                 // Creates a surface for the swapchain, typically platform-specific (e.g., GLFW, Win32, etc.)
    pickPhysicalDevice();            // Selects the physical device (GPU) based on supported features and preferences
    createLogicalDeviceAndQueue();   // Creates a logical device (GPU abstraction) and command queues
    setupDebugMessenger();
    setupDebugMessenger();           // Sets up debugging tools (optional, but very useful for development)
    establishDisplaySizeIdentity();  // Initializes display size and other related parameters

    createSwapChain();               // Creates the swap chain, which manages a collection of images that will be rendered and displayed on the screen
    createImageViews();              // Creates image views for the swapchain images
    createRenderPass();              // Sspecifies how rendering is done
    createDescriptorSetLayouts();     // Creates the descriptor set layout to describe how shaders access resources
    createGraphicsPipeline();        // Creates the graphics pipeline, (specifies shaders and their configuration)
    createFramebuffers();            // Creates framebuffers for each swap chain image
    createCommandPool();             // Creates a command pool for managing command buffers

    decodeImage();
    createTextureImage();
    copyBufferToImage();
    createTextureImageViews();
    createTextureSampler();

    createVertexBuffer();            // Vertex buffers creation
    createIndexBuffer();             // Index buffers creation
    createUniformBuffers();          // Creates uniform buffers for passing data to shaders (MVP matrices)
    createDescriptorPool();          // Creates a descriptor pool to allocate resources like uniform buffers and textures
    createDescriptorSets();          // Creates descriptor sets for shaders to access resources (like uniform buffers)
    createCommandBuffers();          // Creates the command buffer to record drawing commands
    createSyncObjects();             // Creates synchronization objects (like semaphores and fences) for handling GPU synchronization

    initialized = true;              // Marks the Vulkan initialization as complete
}

// -------------------------------------------------------------------------------------------------
// Vulkan instance and device
// -------------------------------------------------------------------------------------------------

/*
 * It is possible to create multiple VkInstances within the same application, for example, if the
 * application needs to use multiple GPUs or create multiple windows.
 */
void HelloVK::createInstance() {
    assert(!enableValidationLayers || checkValidationLayerSupport());
    auto requiredExtensions = getRequiredExtensions(enableValidationLayers);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan app";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    createInfo.pApplicationInfo = &appInfo;

    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        createInfo.enabledLayerCount =
                static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           extensions.data());
    LOGI("available extensions");
    for (const auto &extension: extensions) {
        LOGI("\t %s", extension.extensionName);
    }
}

/*
 * 'createSurface' can only be called after the android ecosystem has had the chance to provide a
 * native window. This happens after the APP_CMD_START event has had a chance to be called.
 *
 * VkSurface which represents the window to render to.
 */
void HelloVK::createSurface() {
    assert(window != nullptr);  // window not initialized
    const VkAndroidSurfaceCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = window.get()};

    VK_CHECK(vkCreateAndroidSurfaceKHR(instance, &create_info, nullptr, &surface));
}

/*
 * Enumerate the physical device (GPUs) available and pick the first suitable device available.
 */
void HelloVK::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    assert(deviceCount > 0);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &d: devices) {
        if (isDeviceSuitable(d)) {
            physicalDevice = d;
            break;
        }
    }

    assert(physicalDevice != VK_NULL_HANDLE);  // failed to find a suitable GPU!
}

/*
 * Check whether the device supports the GRAPHICS queue.
 */
bool HelloVK::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool HelloVK::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    std::set < std::string > requiredExtensions(deviceExtensions.begin(),
                                                deviceExtensions.end());

    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices HelloVK::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily: queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }
    return indices;
}

/*
 * After PhysicalDevice found, create a logical device (known as VkDevice). This represents an
 * initialized Vulkan device that is ready to create all other objects to be used by your
 * application.
 */
void HelloVK::createLogicalDeviceAndQueue() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set < uint32_t > uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                                 indices.presentFamily.value()};
    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount =
            static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount =
            static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
                static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void HelloVK::setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                          &debugMessenger));
}

void HelloVK::establishDisplaySizeIdentity() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    uint32_t width = capabilities.currentExtent.width;
    uint32_t height = capabilities.currentExtent.height;
    if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
        capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        // Swap to get identity width and height
        capabilities.currentExtent.height = width;
        capabilities.currentExtent.width = height;
    }

    displaySizeIdentity = capabilities.currentExtent;
}

/*
 * This function queries the physical device for information about the surface's capabilities,
 * available formats, and present modes, which are required to create a swap chain.
 */
SwapChainSupportDetails HelloVK::querySwapChainSupport(VkPhysicalDevice device) const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                              nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

// -------------------------------------------------------------------------------------------------
// Create Swapchain and sync objects
// -------------------------------------------------------------------------------------------------

/*
 * Sync objects are objects used for synchronization. Vulkan has VkFence, VkSemaphore, and VkEvent
 * which are used to control resource access across multiple queues. These objects are needed if
 * you're using multiple queues and render passes. For our simple example, we wouldn't be using it.
 */
void HelloVK::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));

        VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));

        VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]));
    }
}

/*
 * VkSwapchain is a Vulkan object that represents a queue of images that can be presented to the
 * display. It is used to implement double buffering or triple buffering, which can reduce tearing
 * and improve performance. VkSwapchains are typically created once at the start of the application
 * and destroyed at the end. You'll also need to prepare for swapchain recreation after the device
 * loses context.
 */
void HelloVK::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    auto chooseSwapSurfaceFormat =
            [](const std::vector<VkSurfaceFormatKHR> &availableFormats) {
                for (const auto &availableFormat: availableFormats) {
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                    }
                }
                return availableFormats[0];
            };

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

    // VK_PRESENT_MODE_FIFO_KHR = Hard Vsync --> https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
    // This is always supported on Android phones
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    pretransformFlag = swapChainSupport.capabilities.currentTransform;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = displaySizeIdentity;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = pretransformFlag;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = displaySizeIdentity;
}

void HelloVK::reset(ANativeWindow *newWindow, AAssetManager *newManager) {
    window.reset(newWindow);
    assetManager = newManager;

    if (initialized) {
        createSurface();
        recreateSwapChain();
    }
}

void HelloVK::recreateSwapChain() {
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createFramebuffers();
}

// -------------------------------------------------------------------------------------------------
// Create Renderpass and Framebuffer
// -------------------------------------------------------------------------------------------------

/*
 * VkImageView: Describes how to access a VkImage, specifying the image's subresource range, pixel
 * format, and channel swizzle.
 *
 * VkRenderPass: Defines how the GPU should render a scene, including attachments, rendering order,
 * and usage at each pipeline stage.
 *
 * VkFramebuffer: Represents a set of image views bound as attachments during a render pass's execution.
 *
 * The loop iterates over each image in swapChainImages to create a corresponding image view.
 */
void HelloVK::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i]; // Specifies the Vulkan image to be accessed.
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Sets the image view type as 2D.
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]));
    }
}

/*
 * Attachment in Vulkan is what is usually known as render target, which is usually an image used as
 * output for rendering. Expected to be used when executing a graphics pipeline.
 *
 * A framebuffer (image views container) is bound to this render pass.
 */
void HelloVK::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

/*
 * Framebuffer represents the link to actual images that can be used for attachments (render target).
 * Create a Framebuffer object by specifying the renderpass and the set of image views.
 *
 * Image: represents the actual image data (such as a texture or a render target)
 * View: represents how that data should be interpreted and accessed within shaders
 */
void HelloVK::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]));
    }
}

// -------------------------------------------------------------------------------------------------
// Create Shader and Pipeline
// -------------------------------------------------------------------------------------------------

/*
 * A VkDescriptorSetLayout is the template for a VkDescriptorSet, which is a group of descriptors.
 * The Descriptors are the handle that enable shaders to access resources (such as Buffers, Images,
 * or Samplers).
 */
void HelloVK::createDescriptorSetLayouts() {
    // Set 0: Object UBO (for model, view, proj matrices)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
    objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    objectLayoutInfo.bindingCount = 1;
    objectLayoutInfo.pBindings = &uboLayoutBinding;

    VK_CHECK(vkCreateDescriptorSetLayout(device, &objectLayoutInfo, nullptr,
                                         &objectDescriptorSetLayout));

    // Set 1: Light UBO
    VkDescriptorSetLayoutBinding lightLayoutBinding{};
    lightLayoutBinding.binding = 0;  // Binding = 0 for set = 1
    lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightLayoutBinding.descriptorCount = 1;
    lightLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lightLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo lightLayoutInfo{};
    lightLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lightLayoutInfo.bindingCount = 1;
    lightLayoutInfo.pBindings = &lightLayoutBinding;

    VK_CHECK(vkCreateDescriptorSetLayout(device, &lightLayoutInfo, nullptr,
                                         &lightDescriptorSetLayout));

    // Set 2: Texture (combined image and sampler)
    VkDescriptorSetLayoutBinding textureLayoutBinding{};
    textureLayoutBinding.binding = 0;  // Binding = 0 for set = 2
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureLayoutBinding.descriptorCount = 1;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureLayoutBinding;

    VK_CHECK(vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr,
                                         &textureDescriptorSetLayout));
}

/*
 * Define the createShaderModule function to load in the shaders into VkShaderModule objects
 *
 * A VkShaderModule is a Vulkan object representing a programmable shader. Used to perform
 * operations on graphics data, such as transforming cubeVertices, shading pixels, and computing
 * global effects.
 */
VkShaderModule HelloVK::createShaderModule(const std::vector<uint8_t> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();

    // The allocator in vector ensures worst case requirements
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

/*
 * A VkPipeline is a Vulkan object that represents a programmable graphics pipeline. It is a set of
 * state objects that describe how the GPU should render a scene.
 *
 * Graphics pipeline loading a simple vertex and fragment shader, both with 'main' set as entrypoint.
 * And it has:
 * - the stage configurations
 * - the render pass (that describes all the resources used on the render -> draw call)
 * - the descriptor sets layouts
 * - and the shader modules
 */
void HelloVK::createGraphicsPipeline() {
    auto vertShaderCode = LoadBinaryFileToVector("shaders/shader.vert.spv", assetManager);
    auto fragShaderCode = LoadBinaryFileToVector("shaders/shader.frag.spv", assetManager);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{}; // pass te cubeVertices to shader
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDescriptorSetLayout> setLayouts = {objectDescriptorSetLayout,
                                                     lightDescriptorSetLayout,
                                                     textureDescriptorSetLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCI{};
    dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
    dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateCI;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                       nullptr, &graphicsPipeline));
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

// -------------------------------------------------------------------------------------------------
// DescriptorSet and Uniform Buffer
// -------------------------------------------------------------------------------------------------

/*
 * A VkDescriptorSet is a Vulkan object that represents a collection of descriptor resources.
 * Descriptor resources are used to provide shader inputs, such as uniform buffers, image samplers,
 * and storage buffers. To create the VkDescriptorSets, we will need to create a VkDescriptorPool.
 *
 * A VkBuffer is a memory buffer used for sharing data between the GPU and CPU. When utilized as a
 * Uniform buffer, it passes data to shaders as uniform variables. Uniform variables are constants
 * that can be accessed by all shaders in a pipeline.
 */
void HelloVK::createDescriptorPool() {
    VkDescriptorPoolSize poolSizes[2];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT *
                                                         (DESCRIPTOR_SETS_PER_FRAME -
                                                          1)); // less textures
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 1);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * DESCRIPTOR_SETS_PER_FRAME);

    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
}

/*
 * The descriptor sets describe the resources bound to the binding points in a shader (uniforms, textures)
 *
 * Create VkDescriptorSets allocated from the VkDescriptorPool specified (for the creation of the
 * buffers).
 */
void HelloVK::createDescriptorSets() {
    // Resize descriptor sets arrays
    cubeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    planeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    lightDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    textureDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    // Allocate descriptor sets for object UBO (set = 0)
    std::vector<VkDescriptorSetLayout> objectLayouts(MAX_FRAMES_IN_FLIGHT,
                                                     objectDescriptorSetLayout);
    VkDescriptorSetAllocateInfo cubeAllocInfo{};
    cubeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    cubeAllocInfo.descriptorPool = descriptorPool;
    cubeAllocInfo.descriptorSetCount = static_cast<uint32_t>(objectLayouts.size());
    cubeAllocInfo.pSetLayouts = objectLayouts.data();
    VK_CHECK(vkAllocateDescriptorSets(device, &cubeAllocInfo, cubeDescriptorSets.data()));

    // Allocate descriptor sets for plane UBO (set = 0, similar to cube)
    std::vector<VkDescriptorSetLayout> planeLayouts(MAX_FRAMES_IN_FLIGHT,
                                                    objectDescriptorSetLayout);
    VkDescriptorSetAllocateInfo planeAllocInfo{};
    planeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    planeAllocInfo.descriptorPool = descriptorPool;
    planeAllocInfo.descriptorSetCount = static_cast<uint32_t>(planeLayouts.size());
    planeAllocInfo.pSetLayouts = planeLayouts.data();
    VK_CHECK(vkAllocateDescriptorSets(device, &planeAllocInfo, planeDescriptorSets.data()));

    // Allocate descriptor sets for light UBO (set = 1)
    std::vector<VkDescriptorSetLayout> lightLayouts(MAX_FRAMES_IN_FLIGHT,
                                                    lightDescriptorSetLayout);
    VkDescriptorSetAllocateInfo lightAllocInfo{};
    lightAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    lightAllocInfo.descriptorPool = descriptorPool;
    lightAllocInfo.descriptorSetCount = static_cast<uint32_t>(lightLayouts.size());
    lightAllocInfo.pSetLayouts = lightLayouts.data();
    VK_CHECK(vkAllocateDescriptorSets(device, &lightAllocInfo, lightDescriptorSets.data()));

    // Allocate descriptor sets for textures (set = 2)
    std::vector<VkDescriptorSetLayout> textureLayouts(MAX_FRAMES_IN_FLIGHT,
                                                      textureDescriptorSetLayout);
    VkDescriptorSetAllocateInfo textureAllocInfo{};
    textureAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    textureAllocInfo.descriptorPool = descriptorPool;
    textureAllocInfo.descriptorSetCount = static_cast<uint32_t>(textureLayouts.size());
    textureAllocInfo.pSetLayouts = textureLayouts.data();
    VK_CHECK(vkAllocateDescriptorSets(device, &textureAllocInfo, textureDescriptorSets.data()));

    // Write descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Object UBO (set = 0)
        VkDescriptorBufferInfo cubeBufferInfo{};
        cubeBufferInfo.buffer = cubeUniformBuffers[i]; // Assuming cubeUniformBuffers holds object data
        cubeBufferInfo.offset = 0;
        cubeBufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet vertexsDescriptorWrite{};
        vertexsDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vertexsDescriptorWrite.dstSet = cubeDescriptorSets[i];
        vertexsDescriptorWrite.dstBinding = 0; // Set = 0, Binding = 0
        vertexsDescriptorWrite.dstArrayElement = 0;
        vertexsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vertexsDescriptorWrite.descriptorCount = 1;
        vertexsDescriptorWrite.pBufferInfo = &cubeBufferInfo;

        // Plane UBO (set = 0)
        VkDescriptorBufferInfo planeBufferInfo{};
        planeBufferInfo.buffer = planeUniformBuffers[i];
        planeBufferInfo.offset = 0;
        planeBufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet planeDescriptorWrite{};
        planeDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        planeDescriptorWrite.dstSet = planeDescriptorSets[i];
        planeDescriptorWrite.dstBinding = 0; // Set = 0, Binding = 0
        planeDescriptorWrite.dstArrayElement = 0;
        planeDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        planeDescriptorWrite.descriptorCount = 1;
        planeDescriptorWrite.pBufferInfo = &planeBufferInfo;

        // Light UBO (set = 1)
        VkDescriptorBufferInfo lightBufferInfo{};
        lightBufferInfo.buffer = lightUniformBuffers[i];
        lightBufferInfo.offset = 0;
        lightBufferInfo.range = sizeof(LightUBO);

        VkWriteDescriptorSet lightDescriptorWrite{};
        lightDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightDescriptorWrite.dstSet = lightDescriptorSets[i];
        lightDescriptorWrite.dstBinding = 0; // Set = 1, Binding = 0
        lightDescriptorWrite.dstArrayElement = 0;
        lightDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightDescriptorWrite.descriptorCount = 1;
        lightDescriptorWrite.pBufferInfo = &lightBufferInfo;

        // Texture (set = 2)
        VkDescriptorImageInfo textureImageInfo{};
        textureImageInfo.imageView = textureImageView;
        textureImageInfo.sampler = textureSampler;
        textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet textureDescriptorWrite{};
        textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureDescriptorWrite.dstSet = textureDescriptorSets[i];
        textureDescriptorWrite.dstBinding = 0; // Set = 2, Binding = 0
        textureDescriptorWrite.dstArrayElement = 0;
        textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureDescriptorWrite.descriptorCount = 1;
        textureDescriptorWrite.pImageInfo = &textureImageInfo;

        // Update descriptor sets for cube, plane, light, and texture
        std::array<VkWriteDescriptorSet, DESCRIPTOR_SETS_PER_FRAME> descriptorWrites = {
                vertexsDescriptorWrite,
                planeDescriptorWrite,
                lightDescriptorWrite,
                textureDescriptorWrite};

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

/*
 * Specify our Uniform Buffer struct and create the uniform buffers. 'createBuffer' will allocate
 * the memory from the VkDeviceMemory using vkAllocateMemory and bind the buffer to the memory using
 * vkBindBufferMemory.
 */
void HelloVK::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    cubeUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    cubeUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    planeUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    planeUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    lightUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    lightUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Cube uniform buffer
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     cubeUniformBuffers[i], cubeUniformBuffersMemory[i]);

        // Plane uniform buffer
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     planeUniformBuffers[i], planeUniformBuffersMemory[i]);

        // LightUBO uniform buffer
        createBuffer(sizeof(LightUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     lightUniformBuffers[i], lightUniformBuffersMemory[i]);
    }
}

/*
 * Create a buffer with specified usage and memory properties i.e a uniform buffer which uses
 * HOST_COHERENT memory Upon creation, these buffers will list memory requirements which need to
 * be satisfied by the device in use in order to be created.
 */
void HelloVK::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/*
 * Finds the index of the memory heap which matches a particular buffer's memory requirements.
 * Vulkan manages these requirements as a bitset, in this case expressed through a uint32_t.
 */
uint32_t HelloVK::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    assert(false);
    return -1;
}

void HelloVK::createVertexBuffer() {
    VkDeviceSize cubeBufferSize = sizeof(cubeVertices[0]) * cubeVertices.size();
    VkDeviceSize planeBufferSize = sizeof(planeVertices[0]) * planeVertices.size();
    VkDeviceSize bufferSize = cubeBufferSize + planeBufferSize;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, planeVertices.data(), (size_t) planeBufferSize);
    memcpy(static_cast<char *>(data) + planeBufferSize, cubeVertices.data(),
           (size_t) cubeBufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloVK::createIndexBuffer() {
    VkDeviceSize cubeBufferSize = sizeof(cubeIndices[0]) * cubeIndices.size();
    VkDeviceSize planeBufferSize = sizeof(planeIndices[0]) * planeIndices.size();
    VkDeviceSize bufferSize = cubeBufferSize + planeBufferSize;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, planeIndices.data(), (size_t) planeBufferSize);
    memcpy(static_cast<char *>(data) + planeBufferSize, cubeIndices.data(),
           (size_t) cubeBufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloVK::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer HelloVK::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void HelloVK::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// -------------------------------------------------------------------------------------------------
// Command Pool and Command Buffer: create, record and Draw
// -------------------------------------------------------------------------------------------------

/*
 * VkCommandPool is a simple object that is used to allocate CommandBuffers. It is connected to a
 * specific Queue Family.
 */
void HelloVK::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
}

/*
 * A VkCommandBuffer is a Vulkan object that represents a list of commands that the GPU will execute.
 * It is a low-level object that provides fine-grained control over the GPU.
 */
void HelloVK::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBuffers.size();

    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));
}

/*
 * In Vulkan, commands like drawing and memory transfers are recorded in command buffers instead of
 * being executed directly. This allows for efficient batch processing and supports multi-threaded
 * command recording. Rendering occurs within RenderPasses, which target specific FrameBuffers set
 * up beforehand.
 */
void HelloVK::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;
    VkClearValue clearColor = {{{0.2588f, 0.2863f, 0.2863f, 1.0f}}};
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    // Array of DrawObjects for plane and cube
    std::vector<DrawObject> drawObjects = {
            {
                    static_cast<uint32_t>(planeIndices.size()),
                    0,
                    0,
                    planeDescriptorSets[currentFrame],
                    textureDescriptorSets[currentFrame], // Texture descriptor set for the plane
                    0
            },
            {
                    static_cast<uint32_t>(cubeIndices.size()),
                    static_cast<uint32_t>(sizeof(Vertex) * planeVertices.size()),
                    static_cast<uint32_t>(sizeof(uint16_t) * planeIndices.size()),
                    cubeDescriptorSets[currentFrame],
                    std::nullopt,
                    0
            }
    };

    // Iterate over the objects and draw them
    for (const auto &object: drawObjects) {
        offsets[0] = object.vertexOffset;

        // Bind vertex and index buffers for the object
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, object.indexOffset, VK_INDEX_TYPE_UINT16);

        // Prepare descriptor sets to bind
        std::vector<VkDescriptorSet> descriptorSets = {object.descriptorSet};

        // If there is a texture descriptor set, add it to the list
        if (object.textureDescriptorSet) {
            descriptorSets.push_back(*object.textureDescriptorSet);
        }

        // Bind the descriptor sets (object + texture, if any)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                                static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(),
                                0, nullptr);

        // Draw the object
        vkCmdDrawIndexed(commandBuffer, object.indexCount, 1, object.firstIndex, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void HelloVK::updateCubeUniformBuffer(glm::mat4 model, glm::mat4 view, glm::mat4 proj,
                                      uint32_t currentImage) {
    // Prepare cube transformation
    UniformBufferObject cubeUbo{};
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
            currentTime - startTime).count();
    float amplitude = glm::radians(90.0f); // 90 degrees
    float frequency = 0.5f; // 0.5 Hz (full cycle every 2 seconds)
    float phaseShift = 0.0f; // Start from the left
    float cycleDuration = 4.0f; // Total duration for one complete cycle (2 left-right + 2 up-down)
    float phaseTime = fmod(time, cycleDuration);
    if (phaseTime < 2.0f) {
        float angle =
                amplitude * glm::sin(2.0f * glm::pi<float>() * frequency * phaseTime + phaseShift);
        cubeUbo.model = model * glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        float angle = amplitude * glm::sin(
                2.0f * glm::pi<float>() * frequency * (phaseTime - 2.0f) + phaseShift);
        cubeUbo.model = model * glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    // cubeUbo.model = glm::mat4(1.0f);
    cubeUbo.view = view;
    cubeUbo.proj = proj;

    // Update cube uniform buffer
    void *data;
    vkMapMemory(device, cubeUniformBuffersMemory[currentImage], 0, sizeof(cubeUbo), 0, &data);
    memcpy(data, &cubeUbo, sizeof(cubeUbo));
    vkUnmapMemory(device, cubeUniformBuffersMemory[currentImage]);
}

void HelloVK::updatePlaneUniformBuffer(glm::mat4 model, glm::mat4 view, glm::mat4 proj,
                                       uint32_t currentImage) {
    // Prepare plane transformation
    UniformBufferObject planeUbo{};
    // down the plane in relation to the cube
    planeUbo.model = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.1f, 0.0f));
    planeUbo.view = view;
    planeUbo.proj = proj;

    // Update plane uniform buffer
    void *data;
    vkMapMemory(device, planeUniformBuffersMemory[currentImage], 0, sizeof(planeUbo), 0, &data);
    memcpy(data, &planeUbo, sizeof(planeUbo));
    vkUnmapMemory(device, planeUniformBuffersMemory[currentImage]);
}

void HelloVK::updateLightBuffer(uint32_t currentImage) {
    // Define light properties (directional light in this example)
    LightUBO light{};
    light.position = glm::vec3(0.0f, 5.0f, 0.0f);   // Position (only used for point/spot light)
    light.direction = glm::vec3(0.0f, 1.0f, 0.0f);  // Direction for directional light
    light.color = glm::vec3(1.0f, 1.0f, 1.0f);      // White light
    light.intensity = 1.0f;                                  // Full intensity
    light.constant = 1.0f;                                   // Attenuation constants (for point light)
    light.linear = 0.09f;
    light.quadratic = 0.032f;

    // Update light uniform buffer
    void *data;
    vkMapMemory(device, lightUniformBuffersMemory[currentImage], 0, sizeof(LightUBO), 0, &data);
    memcpy(data, &light, sizeof(LightUBO));
    vkUnmapMemory(device, lightUniformBuffersMemory[currentImage]);
}

/*
 * You may also need to update the Uniform Buffer as for all the vertices we're rendering
 */
void HelloVK::updateUniformBuffer(uint32_t currentImage) {
    // "Global" parameters
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 1.5f, 6.0f),
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    float FOV = glm::radians(65.0f);
    float ratio = static_cast<float>(swapChainExtent.width) /
                  static_cast<float>(swapChainExtent.height);
    glm::mat4 proj = glm::perspective(FOV, ratio, 0.1f, 100.0f);
    proj[1][1] *= -1;// invert the Y-axis component

    updatePlaneUniformBuffer(model, view, proj, currentImage);
    updateCubeUniformBuffer(model, view, proj, currentImage);
    updateLightBuffer(currentImage);
}

/*
 * Get the command buffer you've composed and submit it to the queue.
 */
void HelloVK::render() { // or draw frame
    if (!initialized) {
        return;
    }
    if (orientationChanged) {
        recreateSwapChain();
        orientationChanged = false;
    }

    // Wait until the previous frame's rendering is complete (prevFrame)
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    // Acquire the next available image from the swap chain
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                                            &imageIndex);

    // Handle swap chain recreation if the window is resized or becomes outdated
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    // failed to acquire swap chain image
    assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

    // Update the uniform buffer for the current frame
    updateUniformBuffer(currentFrame);

    // Reset the fence to mark the frame as in progress
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);

    // Record drawing commands into the command buffer
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    // Submit the command buffer for execution
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Specify synchronization: wait for the image to be available
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // Specify the command buffer to execute
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    // Signal that rendering is finished
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Submit the command buffer to the graphics queue
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

    // Present the rendered image to the screen
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    // Handle presentation result and swap chain status
    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_SUBOPTIMAL_KHR) {
        orientationChanged = true;
    } else if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
    } else {
        assert(result == VK_SUCCESS);  // failed to present swap chain image!
    }
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ---------------------------------------------------------------------------------------------
// Validation layer support and Cleaning
// ---------------------------------------------------------------------------------------------

void vkt::HelloVK::decodeImage() {
    std::vector<uint8_t> imageData = LoadBinaryFileToVector("img1.png", assetManager);
    if (imageData.empty()) {
        LOGE("Fail to load image.");
        return;
    }

    const int requiredChannels = 4;
    unsigned char *decodedData = stbi_load_from_memory(imageData.data(),
                                                       imageData.size(), &textureWidth,
                                                       &textureHeight, &textureChannels,
                                                       requiredChannels);
    if (decodedData == nullptr) {
        LOGE("Fail to load image to memory, %s", stbi_failure_reason());
        return;
    }

    if (textureChannels != requiredChannels) {
        textureChannels = requiredChannels;
    }

    size_t imageSize = textureWidth * textureHeight * textureChannels;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = imageSize;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &imgStagingBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, imgStagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imgStagingMemory));
    VK_CHECK(vkBindBufferMemory(device, imgStagingBuffer, imgStagingMemory, 0));

    uint8_t *data;
    VK_CHECK(vkMapMemory(device, imgStagingMemory, 0, memRequirements.size, 0, (void **) &data));
    memcpy(data, decodedData, imageSize);
    vkUnmapMemory(device, imgStagingMemory);

    stbi_image_free(decodedData);
}

void HelloVK::createTextureImage() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = textureWidth;
    imageInfo.extent.height = textureHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &textureImage));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory));

    vkBindImageMemory(device, textureImage, textureImageMemory, 0);
}

void HelloVK::copyBufferToImage() {
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = textureImage;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageExtent.width = textureWidth;
    bufferImageCopy.imageExtent.height = textureHeight;
    bufferImageCopy.imageExtent.depth = 1;
    bufferImageCopy.bufferOffset = 0;

    vkCmdCopyBufferToImage(cmd, imgStagingBuffer, textureImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &bufferImageCopy);

    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &imageMemoryBarrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(graphicsQueue);
}

void HelloVK::createTextureImageViews() {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = textureImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &textureImageView));
}

void HelloVK::createTextureSampler() {
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.maxAnisotropy = 16;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = VK_LOD_CLAMP_NONE;

    VK_CHECK(vkCreateSampler(device, &createInfo, nullptr, &textureSampler));
}

// ---------------------------------------------------------------------------------------------
// Validation layer support and Cleaning
// ---------------------------------------------------------------------------------------------

bool HelloVK::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName: validationLayers) {
        bool layerFound = false;
        for (const auto &layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }
    return true;
}

void HelloVK::cleanupSwapChain() {
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void HelloVK::cleanup() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device, objectDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, lightDescriptorSetLayout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) { // destroy uniforms
        vkDestroyBuffer(device, cubeUniformBuffers[i], nullptr);
        vkFreeMemory(device, cubeUniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, planeUniformBuffers[i], nullptr);
        vkFreeMemory(device, planeUniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, lightUniformBuffers[i], nullptr);
        vkFreeMemory(device, lightUniformBuffersMemory[i], nullptr);
    }
    vkDestroyBuffer(device, imgStagingBuffer, nullptr);
    vkFreeMemory(device, imgStagingMemory, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    initialized = false;
}



