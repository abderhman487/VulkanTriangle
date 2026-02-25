#ifndef VULKAN_H
#define VULKAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>


/* Structs definitions */

typedef uint32_t u32;
typedef uint8_t u8;

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

    VkFramebuffer *swapChainFramebuffers;

    VkCommandPool commandPool;
    VkCommandBuffer *commandBuffers;
    u32 commandBufferCount;

    VkSemaphore *imageAvailableSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;

    u32 imageAvailableSemaphoreCount;
    u32 renderFinishedSemaphoreCount;
    u32 inFlightFenceCount;
    
    
    u32 currentFrame;
    bool framebufferResized;
} App;

typedef struct shaderFile{
    size_t size;
    char *code;
} shaderFile;

/* functions prototype */

void initWindow(App *pApp);
void initVulkan(App *pApp);
void mainloop(App *pApp);
void cleanup(App *pApp);

void createInstance(App *pApp);

bool checkValidationLayerSupport(void);

u8 verifyExtensionsSupport(
                        u32 glfwExtensionCount,
                        const char **glfwExtensions,
                        u32 extensionCount,
                        VkExtensionProperties *extensions
                        );

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

void createFramebuffers(App *pApp);

void createCommandPool(App *pApp);

void createCommandbuffers(App *pApp);

void recordCommandBuffer(App *pApp, VkCommandBuffer commandBuffer, u32 imageIndex);

void drawFrame(App *pApp);

void createSyncObjects(App *pApp);

void recreateSwapChain(App *pApp);

void cleanupSwapChain(App *pApp);

static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


#endif