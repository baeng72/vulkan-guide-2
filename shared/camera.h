#pragma once
#include <vk_types.h>

class Camera{
    public:
    vec3 velocity;
    vec3 position;
    //vertical rotation
    float pitch{0.f};
    //horizonatal rotatin
    float yaw{0.f};

    mat4 getViewMatrix();
    mat4 getRotationMatrix();

    void update();
    void processKey(i32 key, i32 scancode, bool pressed, bool repeat);
    void processMouseMove(float xrel, float yrel);    
};