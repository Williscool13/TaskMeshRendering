//
// Created by William on 2025-12-04.
//

#ifndef TASKMESHRENDERING_VK_TYPES_H
#define TASKMESHRENDERING_VK_TYPES_H

#include <glm/glm.hpp>

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

    glm::mat4 invView{1.0f};
    glm::mat4 invProj{1.0f};
    glm::mat4 invViewProj{1.0f};

    glm::mat4 viewProjCameraLookDirection{1.0f};

    glm::mat4 prevView{1.0f};
    glm::mat4 prevProj{1.0f};
    glm::mat4 prevViewProj{1.0f};

    glm::mat4 prevInvView{1.0f};
    glm::mat4 prevInvProj{1.0f};
    glm::mat4 prevInvViewProj{1.0f};

    glm::mat4 prevViewProjCameraLookDirection{1.0f};

    glm::vec4 cameraWorldPos{0.0f};
    glm::vec4 prevCameraWorldPos{0.0f};

    Frustum frustum{};

    glm::vec2 renderTargetSize{};
    glm::vec2 texelSize{};

    glm::vec2 cameraPlanes{1000.0f, 0.1f};

    float deltaTime{};
};


#endif //TASKMESHRENDERING_VK_TYPES_H