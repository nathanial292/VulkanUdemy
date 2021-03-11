#pragma once
#include "Light.h"

namespace vulkan {
    struct UniformLight {
        glm::vec3 colour;
        glm::vec3 direction;
        float ambientIntensity;
        float diffuseIntensity;
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

        void UpdatePosition(float xDir, float yDir, float zDir);

        ~DirectionalLight();

    private:
        glm::vec3 direction;
    };
}