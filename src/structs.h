struct QueueFamilyIndices
{
    std::optional<uint32_t> graphic;
    std::optional<uint32_t> present;
    std::optional<uint32_t> transfer;

    bool isComplete()
    {
        return graphic.has_value() && present.has_value() && transfer.has_value();
    }
};

struct SwapChainProperties
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Format;
    std::vector<VkPresentModeKHR> PresentModes;
};

struct SwapChain
{
    VkSwapchainKHR Swapchain;
    VkSurfaceCapabilitiesKHR Capabilitie;
    VkFormat Format;
    VkPresentModeKHR PresentMode;
};

struct Vertex
{
    glm::vec3 coordinate;
    glm::vec4 color;
    glm::vec2 texture;
    static VkVertexInputBindingDescription InputBindingDescription()
    {
        VkVertexInputBindingDescription VertexInputBindingDescription{};
        VertexInputBindingDescription.binding   = 0;
        VertexInputBindingDescription.stride    = sizeof( Vertex );
        VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return VertexInputBindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> InputAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 3> VertexInputAttributeDescription{};
        VertexInputAttributeDescription[ 0 ].binding  = 0;
        VertexInputAttributeDescription[ 0 ].location = 0;
        VertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
        VertexInputAttributeDescription[ 0 ].offset   = offsetof( Vertex, coordinate );

        VertexInputAttributeDescription[ 1 ].binding  = 0;
        VertexInputAttributeDescription[ 1 ].location = 1;
        VertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        VertexInputAttributeDescription[ 1 ].offset   = offsetof( Vertex, color );

        VertexInputAttributeDescription[ 2 ].binding  = 0;
        VertexInputAttributeDescription[ 2 ].location = 2;
        VertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_R32G32_SFLOAT;
        VertexInputAttributeDescription[ 2 ].offset   = offsetof( Vertex, texture );
        return VertexInputAttributeDescription;
    }
    bool operator==( const Vertex &other ) const
    {
        return coordinate == other.coordinate && color == other.color && texture == other.texture;
    }
};

struct Model
{
    std::vector<Vertex> ModelVertecies;
    std::vector<uint32_t> ModelVerteciesIndices;
    uint32_t VerteciesOffset{};
    uint32_t IndeciesOffset{};
};

struct DemensionsUniformrObject
{
    alignas( 16 ) glm::mat4 model;
    alignas( 16 ) glm::mat4 view;
    alignas( 16 ) glm::mat4 proj;
};