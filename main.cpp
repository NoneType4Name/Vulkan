#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
using namespace std;


class HelloTriangleApplication {
public:
    const uint16_t WIDTH{1000};
    const uint16_t HEIGHT{1000};

    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;
    VkInstance instance;
    void initWindow(){
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

    }

    void initVulkan() {
        CreateVKinstance();

    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)){
            glfwPollEvents();
            }
        
    }

    void cleanup() {
        vkDestroyInstance(instance, NULL);
        glfwDestroyWindow(window);
        glfwTerminate();

    }

    void CreateVKinstance(){
        uint32_t glfwExtensionsCount{0};
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        // Additional funcs (may be include it from H file?).
        // as func name: void ChekForSupportAllExtensions.
        uint32_t glfwAvailableExtensionsCount{0};
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, nullptr);
        vector<VkExtensionProperties> AvailableExtensions(glfwAvailableExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, AvailableExtensions.data());
        for (const auto& AvailableExtension : AvailableExtensions){
            // Extension is Available?.
        }

        //

        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = "Vulkan start";
        AppInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        AppInfo.apiVersion = VK_API_VERSION_1_0;
        // AppInfo.pNext = ;

        VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        CreateInfo.pApplicationInfo = &AppInfo;
        CreateInfo.enabledExtensionCount = glfwExtensionsCount;
        CreateInfo.ppEnabledExtensionNames = glfwExtensions;
        CreateInfo.enabledLayerCount = 0;
        if (vkCreateInstance(&CreateInfo, nullptr, &instance) != VK_SUCCESS){
            throw runtime_error("Failed create instance.");
        }
    }
};

int main(int argc, char *argv[]) {
    HelloTriangleApplication app;
    try {
        app.run();
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// test for map from py:

// auto some_function = [](auto&& x) { cout << x << endl;};
// auto the_tuple = make_tuple(1, 2.1, '3', "four");
// apply([&](auto& ...x){(..., some_function(x));}, the_tuple);