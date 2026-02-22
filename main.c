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
} App;

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    VkPresentModeKHR *presentModes;
    u32 formatCount;
    u32 presentModeCount;
} SwapChainSupportDetails;

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
}

void mainloop(App *pApp){
    while(!glfwWindowShouldClose(pApp->window)){
        glfwPollEvents();
    }
}

void cleanup(App *pApp){
    if(enableValidationLayers){
        DestroyDebugUtilsMessengerEXT(pApp->instance, pApp->debugMessenger, NULL);
    }
    vkDestroySwapchainKHR(pApp->device, pApp->swapChain, NULL);

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

    if (vkCreateSwapchainKHR(pApp -> device, &createInfo, NULL, &pApp->swapChain) != VK_SUCCESS) {
        printf("failed to create swap chain!");
        exit(6);
    }
}