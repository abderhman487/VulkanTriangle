#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef uint32_t u32;
typedef uint8_t u8;


const u32 WIN_WIDTH = 800;
const u32 WIN_HEIGHT = 600;
const char *WIN_TITLE = "Triangle";

const u32 validationLayersCount = 1;
const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

const u32 deviceExtensionsCount = 1;
const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    VkPresentModeKHR *presentModes;
    u32 formatCount;
    u32 presentModeCount;
} SwapChainSupportDetails;

typedef struct QueueFamilyIndices{
    u32 graphicsFamily;
    u32 presentFamily;
    bool isGraphicsFamilySet;
    bool isPresentFamilySet;
} QueueFamilyIndices;

typedef struct App {
    GLFWwindow *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilyIndices;
    VkDevice device; //Logical Device
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSwapchainKHR swapChain;
    VkImage *swapChainImages;
    u32 swapChainImageCount;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkImageView *swapChainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

} App;

typedef struct shaderFile{
    size_t size;
    char *code;
} shaderFile;


void initWindow(App *pApp);
void initVulkan(App *pApp);
void mainloop(App *pApp);
void cleanup(App *pApp);

void createInstance(App *pApp);

bool checkValidationLayerSupport(void);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
);

void setupDebugMessenger(App *pApp);
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, 
    const VkAllocationCallbacks *pAllocator, 
    VkDebugUtilsMessengerEXT *pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks *pAllocator);

void pickPhysicalDevice(App *pApp);

u32 rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

void createLogicalDevice(App *pApp);

void createSurface(App *pApp);

bool checkDeviceExtensionSupport(VkPhysicalDevice device);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(u32 formatCount, VkSurfaceFormatKHR *availableFormats);

VkPresentModeKHR chooseSwapPresentMode(u32 presentModeCount, VkPresentModeKHR *availablePresentModes);

void createSwapChain(App *pApp);

void createImageViews(App *pApp);

void createGraphicsPipeline(App *pApp);

static shaderFile readFile(char *filename);

VkShaderModule createShaderModule(shaderFile shaderFile, App *pApp);

void createRenderPass(App *pApp);


//===================================================================
int main(void){
    App window = {0};

    initWindow(&window);
    initVulkan(&window);
    mainloop(&window);
    cleanup(&window);

    return 0;
}


void initWindow(App *pApp){
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pApp->window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
}

void initVulkan(App *pApp){
    createInstance(pApp);
    setupDebugMessenger(pApp);
    createSurface(pApp);
    pickPhysicalDevice(pApp);
    createLogicalDevice(pApp);
    createSwapChain(pApp);
    createImageViews(pApp);
    createRenderPass(pApp);
    createGraphicsPipeline(pApp);
}

void mainloop(App *pApp){
    while(!glfwWindowShouldClose(pApp->window)){
        glfwPollEvents();
    }
}

void cleanup(App *pApp){

    vkDestroyPipeline(pApp->device, pApp->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pApp->device, pApp->pipelineLayout, NULL);
    vkDestroyRenderPass(pApp->device, pApp->renderPass, NULL);

    for (u32 i = 0; i < pApp->swapChainImageCount; i++) {
        vkDestroyImageView(pApp->device, pApp->swapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(pApp->device, pApp->swapChain, NULL);

    if(enableValidationLayers){
        DestroyDebugUtilsMessengerEXT(pApp->instance, pApp->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(pApp->instance , pApp->surface, NULL);

    vkDestroyDevice(pApp->device, NULL);

    vkDestroyInstance(pApp->instance, NULL);

    glfwDestroyWindow(pApp->window);

    glfwTerminate();
}

u8 verifyExtensionsSupport(
                        u32 glfwExtensionCount,
                        const char **glfwExtensions,
                        u32 extensionCount,
                        VkExtensionProperties *extensions
                        );

void createInstance(App *pApp){

    if(enableValidationLayers && !checkValidationLayerSupport()){
        printf("Validation layers requested but not available!\n");
        exit(1);
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = WIN_TITLE,
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_0,
        .pNext = NULL
    };

    u32 glfwExtensionCount = 0;
    const char **availableGlfwExtensions;

    availableGlfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    const char **glfwExtensions = (const char **) malloc(
        sizeof(char *) * (glfwExtensionCount + 1)
    );

    for(int i = 0; i < glfwExtensionCount; i++)
        glfwExtensions[i] = availableGlfwExtensions[i];
    if(enableValidationLayers){
        glfwExtensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };
    if(enableValidationLayers){
        createInfo.enabledLayerCount = (u32) validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledExtensionCount = glfwExtensionCount+1;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }else{
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledExtensionNames = availableGlfwExtensions;
        createInfo.enabledExtensionCount = glfwExtensionCount;
    }

    if(vkCreateInstance(&createInfo, NULL, &pApp->instance) != VK_SUCCESS){
        printf("Failed to create Vulkan Instance\n");
        exit(1);
    }

    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

    VkExtensionProperties *extensions = (VkExtensionProperties *) 
    malloc(sizeof(VkExtensionProperties) * extensionCount);

    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

    if(!(verifyExtensionsSupport(glfwExtensionCount, glfwExtensions, 
        extensionCount, extensions) > 0)){
            printf("Missing extensions support\n");
            free(extensions);
            exit(1);    
        }
    free(extensions);
    free(glfwExtensions);
}


u8 verifyExtensionsSupport(
                        u32 glfwExtensionCount,
                        const char **glfwExtensions,
                        u32 extensionCount,
                        VkExtensionProperties *extensions
                        )
    {
        u32 foundExtensions = 0;
        for(int i = 0; i < glfwExtensionCount; i++){
            for(int j = 0; j < extensionCount; j++){
                if(strcmp(glfwExtensions[i], extensions[j].extensionName) == 0){
                    foundExtensions++;
                    break;
                }
            }
        }
        return foundExtensions == glfwExtensionCount;
}   


bool checkValidationLayerSupport(void){
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties *availableLayers = (VkLayerProperties *) malloc(
        sizeof(VkLayerProperties) * layerCount);

    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for(int i = 0; i < validationLayersCount; i++){
        bool layerFound = false;
        for(int j = 0; j < layerCount; j++){
            if(strcmp(validationLayers[i], availableLayers[j].layerName) == 0){
                layerFound = true;
                break;
            }
        }
        if(!layerFound){
            free(availableLayers);
            return false;
        }
    }
    free(availableLayers);
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
){
    printf("Validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}


void setupDebugMessenger(App *pApp){
    if(!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    populateDebugMessengerCreateInfo(&createInfo);

    if (CreateDebugUtilsMessengerEXT(pApp->instance, &createInfo, NULL, &pApp->debugMessenger) != VK_SUCCESS) {
        printf("Failed to setup debug messenger!\n");
        exit(2);
    }

}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo) {
  createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo->pfnUserCallback = debugCallback;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, 
    const VkAllocationCallbacks *pAllocator, 
    VkDebugUtilsMessengerEXT *pDebugMessenger) {

  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) 
  vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

  if (func != NULL) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks *pAllocator) {
  
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) 
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        
        if (func != NULL) {
            func(instance, debugMessenger, pAllocator);
        }
}

void pickPhysicalDevice(App *pApp) {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, NULL);

    if(deviceCount == 0){
        printf("failed to find GPUs with Vulkan support!\n");
        exit(3);
    }

    VkPhysicalDevice *devices = (VkPhysicalDevice *) malloc(
        sizeof(VkPhysicalDevice) * deviceCount
    );

    vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, devices);

    u32 deviceScore = 0;
    VkPhysicalDevice device = VK_NULL_HANDLE;;

    for(int i = 0; i < deviceCount; i++){
        u32 score = rateDeviceSuitability(devices[i], pApp->surface);
        if(score > deviceScore){
            deviceScore = score;
            device = devices[i];
        }
    }
    if(device == VK_NULL_HANDLE){
        printf("failed to find a suitable GPU!\n");
        exit(3);
    }

    pApp->physicalDevice = device;
    printf("GPU selected\n");

    pApp->queueFamilyIndices = findQueueFamilies(device, pApp->surface);
    free(devices);
}

u32 rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface){
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    u32 score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
       return 0;
    }

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if(!indices.isGraphicsFamilySet){
        printf("Queue family is not supported!\n");
        return 0;
    }

    bool extensionsSupported = checkDeviceExtensionSupport(device);
    if(!extensionsSupported){
        printf("required device extensions is not supported!\n");
        return 0;
    }

    bool swapChainAdequate = false;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    if(swapChainSupport.formatCount == 0 || swapChainSupport.presentModeCount == 0){
        printf("swap chain not adequately supported!\n");
        return 0;
    }

    return score;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface){
    QueueFamilyIndices indices = {0};

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilyProperties = malloc(
        sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);

    for(int i = 0; i < queueFamilyCount; i++){
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = i;
            indices.isGraphicsFamilySet = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport){
            indices.presentFamily = i;
            indices.isPresentFamilySet = true;
        }
        if(indices.isGraphicsFamilySet && indices.isPresentFamilySet)
            break;
    }


    free(queueFamilyProperties);
    return indices;
}

void getFamilyDeviceQueues(VkDeviceQueueCreateInfo *queues, QueueFamilyIndices indices, VkPhysicalDeviceFeatures deviceFeatures) {
  // GraphicsQueue
  VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = indices.graphicsFamily,
    .queueCount = 1
  };

  float graphicsQueuePriority = 1.0f;
  graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
  queues[0] = graphicsQueueCreateInfo;

  // PresentQueue
  VkDeviceQueueCreateInfo presentQueueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = indices.presentFamily,
    .queueCount = 1
  };

  float presentQueuePriority = 1.0f;
  graphicsQueueCreateInfo.pQueuePriorities = &presentQueuePriority;
  queues[1] = presentQueueCreateInfo;
}

void createLogicalDevice(App *pApp){
    QueueFamilyIndices indices = findQueueFamilies(pApp->physicalDevice, pApp->surface);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(pApp->physicalDevice, &deviceFeatures);

    VkDeviceQueueCreateInfo queues[2];
    getFamilyDeviceQueues(queues, indices, deviceFeatures);

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queues,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = deviceExtensionsCount,
        .ppEnabledExtensionNames = deviceExtensions,
    };

    if(enableValidationLayers){
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    }
    else{
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(pApp->physicalDevice, &createInfo, NULL, &pApp->device) != VK_SUCCESS){
        printf("failed to create logical device!\n");
        exit(4);
    }

    vkGetDeviceQueue(pApp->device, pApp->queueFamilyIndices.graphicsFamily,0 , &pApp->graphicsQueue);
}

void createSurface(App *pApp){
    if(glfwCreateWindowSurface(pApp->instance, pApp->window, 
        NULL, &pApp->surface) != VK_SUCCESS){
            printf("failed to create window sufrace!\n");
            exit(5);
        }
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device){
    u32 extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions = (VkExtensionProperties *) malloc(
        sizeof(VkExtensionProperties) * extensionCount
    );
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

    for(u32 i = 0; i < deviceExtensionsCount; i++){
        bool extensionFound = false;
        for(u32 j = 0; j < extensionCount; j++){
            if(strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0){
                extensionFound = true;
                break;
            }
        }
        if(!extensionFound){
            free(availableExtensions);
            return false;
        }
    }
    free(availableExtensions);
    return true;
}


// Swap Chain
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface){
    SwapChainSupportDetails details = {0};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &formatCount, NULL);
    details.formatCount = formatCount;

    if(formatCount != 0){
        details.formats = details.formats = (VkSurfaceFormatKHR *) malloc(
            sizeof(VkSurfaceFormatKHR) * formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
    details.presentModeCount = presentModeCount;

    if(presentModeCount != 0){
        details.presentModes = (VkPresentModeKHR *) malloc(
            sizeof(VkPresentModeKHR) * presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);
    }

    return details;
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(u32 formatCount,VkSurfaceFormatKHR *availableFormats) {
    for(int i = 0; i < formatCount; i++){
        if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return availableFormats[i];
        }
    }    
    
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(u32 presentModeCount, VkPresentModeKHR *availablePresentModes) {
    for(int i = 0; i < presentModeCount; i++){
        if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
            return availablePresentModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

u32 clamp_u32(u32 num, u32 min, u32 max){
    if(num < min) return min;
    if(num > max) return max;
    return num;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {(u32) width, (u32) height};

    actualExtent.width = clamp_u32(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = clamp_u32(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void createSwapChain(App *pApp){
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pApp->physicalDevice, pApp->surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formatCount, 
        swapChainSupport.formats);

    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModeCount,
    swapChainSupport.presentModes);

    VkExtent2D extent = chooseSwapExtent(pApp->window, swapChainSupport.capabilities);

    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.minImageCount > 0 && 
        imageCount > swapChainSupport.capabilities.minImageCount){
            imageCount = swapChainSupport.capabilities.minImageCount;
        }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = pApp->surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    QueueFamilyIndices indices = findQueueFamilies(pApp->physicalDevice, pApp->surface);
    u32 queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }

    if (vkCreateSwapchainKHR(pApp->device, &createInfo, NULL, &pApp->swapChain) != VK_SUCCESS) {
        printf("failed to create swap chain!");
        exit(6);
    }

    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, NULL);
    pApp->swapChainImages = (VkImage *) malloc(sizeof(VkImage) *imageCount);
    vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, pApp->swapChainImages);
    
    pApp->swapChainImageFormat = surfaceFormat.format;
    pApp->swapChainExtent = extent;
    pApp->swapChainImageCount = imageCount;
}


void createImageViews(App *pApp){
    pApp->swapChainImageViews = (VkImageView *) malloc(
        sizeof(VkImageView) * pApp->swapChainImageCount);

    for(u32 i = 0; i < pApp->swapChainImageCount; i++){
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = pApp->swapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = pApp->swapChainImageFormat,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        if(vkCreateImageView(pApp->device, &createInfo, NULL, 
            &pApp->swapChainImageViews[i]) != VK_SUCCESS){
                printf("failed to crate image views!\n");
                exit(7);
            }
    }
}


// Graphic Pipelines
void createGraphicsPipeline(App *pApp) {
    shaderFile vertShaderFile = readFile("./shaders/vert.spv");
    shaderFile fragShaderFile = readFile("./shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderFile, pApp);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderFile, pApp);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL, // Optional
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL, // Optional
    };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        viewport.x = 0.0f,
        viewport.y = 0.0f,
        viewport.width = (float) pApp->swapChainExtent.width,
        viewport.height = (float) pApp->swapChainExtent.height,
        viewport.minDepth = 0.0f,
        viewport.maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = pApp->swapChainExtent,
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    u32 dynamicStatesCount = 2;

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (u32) dynamicStatesCount,
        .pDynamicStates = dynamicStates,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp = 0.0f, // Optional
        .depthBiasSlopeFactor = 0.0f, // Optional
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f, // Optional
        .pSampleMask = NULL, // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE, // Optional        
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp = VK_BLEND_OP_ADD, // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
    };

    VkPipelineColorBlendStateCreateInfo colorBlending= {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants[0] = 0.0f, // Optional
        .blendConstants[1] = 0.0f, // Optional
        .blendConstants[2] = 0.0f, // Optional
        .blendConstants[3] = 0.0f, // Optional
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0, // Optional
        .pSetLayouts = NULL, // Optional
        .pushConstantRangeCount = 0, // Optional
        .pPushConstantRanges = NULL, // Optional
    };

    if (vkCreatePipelineLayout(pApp->device, &pipelineLayoutInfo, NULL, &pApp->pipelineLayout) != VK_SUCCESS) {
        printf("failed to create pipeline layout!\n");
        exit(8);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = NULL, // Optional
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pApp->pipelineLayout,
        .renderPass = pApp->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1, // Optional
    };
    if (vkCreateGraphicsPipelines(pApp->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pApp->graphicsPipeline) != VK_SUCCESS) {
        printf("failed to create graphics pipeline!\n");
        exit(8);
    }

    free(fragShaderFile.code);
    free(vertShaderFile.code);

    vkDestroyShaderModule(pApp->device, fragShaderModule, NULL);
    vkDestroyShaderModule(pApp->device, vertShaderModule, NULL);
}

static shaderFile readFile(char *filename){

    FILE *file;
    if((file = fopen(filename,"rb")) == NULL){
        printf("failed to open %s\n", filename);
        exit(8);
    }

    fseek(file,0L,SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *buffer = (char *)malloc(size);
    if(buffer == NULL){
        printf("can't allocate buffer to read the shader binary file!\n");
        exit(8);
    }
    fread(buffer, size, sizeof(char), file);

    fclose(file);

    shaderFile shaderFile = {
        .code = buffer,
        .size = size
    };
    return shaderFile;
}

VkShaderModule createShaderModule(shaderFile shaderFile, App *pApp) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderFile.size,
        .pCode = (u32 *) shaderFile.code,
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pApp->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("failed to create shader module!\n");
        exit(8);
    }

    return shaderModule;
}


void createRenderPass(App *pApp){
    VkAttachmentDescription colorAttachment = {
        .format = pApp->swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    if (vkCreateRenderPass(pApp->device, &renderPassInfo, NULL, &pApp->renderPass) != VK_SUCCESS) {
        printf("failed to create render pass!\n");
        exit(9);
    }
}