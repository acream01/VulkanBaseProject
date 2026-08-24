#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "Vertex.hpp"

class Geometry {
public:
    Geometry(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue,
        std::vector<Vertex> vertices, std::vector<uint32_t> indices);
    ~Geometry();

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    VkBuffer getVertexBuffer() const { return vertexBuffer; }
    VkBuffer getIndexBuffer() const { return indexBuffer; }
    uint32_t getIndexCount() const { return static_cast<uint32_t>(indices.size()); }
    const std::vector<Vertex>& getVertices() const { return vertices; }

private:
    VkDevice device; // non-owning

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    void createVertexBuffer(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    void createIndexBuffer(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
};

// Generation / loading — pure CPU-side, no Vulkan handles. Free functions.
void generateGrid(uint32_t gridSizeX, uint32_t gridSizeY, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
void loadModel(const std::string& modelPath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);