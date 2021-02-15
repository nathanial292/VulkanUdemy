#pragma once
#include "Light.h"

struct UniformLight {
    glm::vec3 colour;
    glm::vec3 direction;
    float diffuseIntensity;
    float ambientIntensity;
};

class DirectionalLight :
    public Light
{
public:
    DirectionalLight();

    DirectionalLight(float red, float green, float blue,
        float aIntensity, float dIntensity,
        float xDir, float yDir, float zDir);

    UniformLight getLight();

    void UseLight(uint32_t ambientIntensityLocation, uint32_t ambientColourLocation,
        uint32_t diffuseIntensityLocation, uint32_t directionLocation);

    void UpdatePosition(float xDir, float yDir, float zDir);

    ~DirectionalLight();

private:
    glm::vec3 direction;
};

