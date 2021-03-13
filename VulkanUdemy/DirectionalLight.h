#pragma once
#include "Light.h"

namespace vulkan {
    struct UniformLight {
        glm::vec3 direction;
        glm::vec3 colour;
        float ambientIntensity;
        float diffuseIntensity;
    };

    class DirectionalLight :
        public Light
    {
    public:
        DirectionalLight();

        DirectionalLight(glm::vec3 position, glm::vec3 colour, float aIntensity, float dIntensity);

        UniformLight getLight();

        void updateLight(glm::vec3* position, glm::vec3* colour, float *ambientIntensity, float *diffuseIntensity);

        ~DirectionalLight();

    private:
        glm::vec3 direction;
    };
}