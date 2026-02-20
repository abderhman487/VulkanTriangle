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
const char *WIN_TITLE = "Hello World";

const u32 validationLayersCount = 1;
const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

typedef struct App {
    GLFWwindow *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
} App;

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