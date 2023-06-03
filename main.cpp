#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <format>

// #include <P7_Client.h>
using namespace std;

class HelloTriangleApplication
{
public:
    const uint16_t WIDTH{1000};
    const uint16_t HEIGHT{1000};
    const vector<const char *> ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
    const bool EnableValidationLayers = false;
#else
    const bool EnableValidationLayers = true;
#endif

    void run()
    {
        initVulkan();
        initWindow();
        CheckValidationLayers();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window;
    VkInstance instance;

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        CreateVKinstance();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyInstance(instance, NULL);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void CreateVKinstance()
    {
        if (EnableValidationLayers && !CheckValidationLayers())
            throw runtime_error("Unavilable validation layer."); // VLayers

        uint32_t glfwExtensionsCount{0};
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        // Additional funcs (may be include it from H file?).
        // as func name: void ChekForSupportAllExtensions.
        uint32_t glfwAvailableExtensionsCount{0};
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, nullptr);
        vector<VkExtensionProperties> AvailableExtensions(glfwAvailableExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwAvailableExtensionsCount, AvailableExtensions.data());
        // Extension is Available?.

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
        if (EnableValidationLayers)
        {
            CreateInfo.enabledLayerCount = uint32_t(ValidationLayers.size());
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }
        vector<const char*> Extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        CreateInfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
        CreateInfo.ppEnabledExtensionNames = Extensions.data();
        VkResult InstanceCreateCode = vkCreateInstance(&CreateInfo, nullptr, &instance);
        if (InstanceCreateCode != VK_SUCCESS)
        {   
            cout << InstanceCreateCode << endl;
            throw runtime_error(format("{}: {}", "Failed create instance, error code", string_VkResult(InstanceCreateCode)));
        }
    }

    bool CheckValidationLayers()
    {
        uint32_t Count;
        vkEnumerateInstanceLayerProperties(&Count, NULL);
        vector<VkLayerProperties> AviableLayers(Count);
        vkEnumerateInstanceLayerProperties(&Count, AviableLayers.data());
        for (const char *lName : ValidationLayers)
        {
            bool SupportLayer = false;
            for (const auto &AlName : AviableLayers)
            {
                if (!strcmp(lName, AlName.layerName))
                {
                    SupportLayer = true;
                    break;
                }
            }
            if (!SupportLayer)
            {
                cout << "Validation layer " << lName << " isn't support.";
                return false;
            }
        }
        return true;
    }
};

int main(int argc, char *argv[])
{
    HelloTriangleApplication app;
    try
    {
        app.run();
    }
    catch (const exception &e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// test for map from py:

// auto some_function = [](auto&& x) { cout << x << endl;};
// auto the_tuple = make_tuple(1, 2.1, '3', "four");
// apply([&](auto& ...x){(..., some_function(x));}, the_tuple);