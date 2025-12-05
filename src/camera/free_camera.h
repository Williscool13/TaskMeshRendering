//
// Created by William on 2025-11-06.
//

#ifndef WILLENGINETESTBED_FREE_CAMERA_H
#define WILLENGINETESTBED_FREE_CAMERA_H
#include "camera.h"

class FreeCamera final : public Camera
{
public:
    explicit FreeCamera();

    FreeCamera(glm::vec3 startingPosition, glm::vec3 startingLookPoint);

    ~FreeCamera() override = default;

    void Update(float deltaTime) override;

public:
    float speed{1.0f};
};

#endif //WILLENGINETESTBED_FREE_CAMERA_H