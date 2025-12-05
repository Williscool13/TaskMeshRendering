//
// Created by William on 2025-12-04.
//

#ifndef TASKMESHRENDERING_VK_TYPES_H
#define TASKMESHRENDERING_VK_TYPES_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform
{
    glm::vec3 translation{};
    glm::quat rotation{};
    glm::vec3 scale{};

    [[nodiscard]] glm::mat4 GetMatrix() const { return glm::translate(glm::mat4(1.0f), translation) * mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale); }

    static const Transform Identity;
};

inline const Transform Transform::Identity{
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f}
};

struct Frustum
{
    glm::vec4 planes[6];

    Frustum() = default;

    explicit Frustum(const glm::mat4& viewProj);
};

struct SceneData
{
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 viewProj{1.0f};

    glm::vec4 cameraWorldPos{0.0f};

    Frustum frustum{};

    glm::vec2 renderTargetSize{};
    glm::vec2 texelSize{};

    float deltaTime{};
};

struct Vertex
{
    glm::vec3 position{0.0f};
};

struct Meshlet
{
    glm::vec4 meshletBoundingSphere;

    glm::vec3 coneApex;
    float coneCutoff;

    glm::vec3 coneAxis;
    uint32_t vertexOffset;

    uint32_t meshletVerticesOffset;
    uint32_t meshletTriangleOffset;
    uint32_t meshletVerticesCount;
    uint32_t meshletTriangleCount;
};

struct MeshletPrimitive
{
    uint32_t meshletOffset{0};
    uint32_t meshletCount{0};
    uint32_t materialIndex{0};
    uint32_t padding1{0};
    // {3} center, {1} radius
    glm::vec4 boundingSphere{};
};

struct ExtractedMeshletModel
{
    std::string name{};
    bool bSuccessfullyLoaded{false};

    std::vector<Vertex> vertices{};
    std::vector<uint32_t> meshletVertices{};
    std::vector<uint8_t> meshletTriangles{};
    std::vector<Meshlet> meshlets{};

    MeshletPrimitive primitive{};
    Transform transform{};
};


#endif //TASKMESHRENDERING_VK_TYPES_H