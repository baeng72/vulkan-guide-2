#include <camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>

void Camera::update(){
    mat4 cameraRotation = getRotationMatrix();
    position += vec3(cameraRotation * vec4(velocity * 0.5f, 0.f));
}

void Camera::processMouseMove(float xrel, float yrel){
    yaw += (float)(xrel/200.f);
    pitch -= (float)(yrel/200.f);
}

void Camera::processKey(i32 key, i32 scancode, bool pressed, bool repeat){
    switch(key){
        case GLFW_KEY_W:
        velocity.z = pressed ? -1.f : 0.f;
        break;
        case GLFW_KEY_S:
        velocity.z = pressed ? 1.f : 0.f;
        break;
        case GLFW_KEY_A:
        velocity.x = pressed ? -1.f : 0.f;
        break;
        case GLFW_KEY_D:
        velocity.x = pressed ? 1.f : 0.f;
        break;
    }
}

mat4 Camera::getViewMatrix(){
    //to create a correct model view, we need to move the world in opposite direction
    //to the camera so we will create the camera model matrix and invert
    mat4 cameraTranslation = glm::translate(mat4(1.f), position);
    mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

mat4 Camera::getRotationMatrix(){
    //fairly typical FPS style camera. we join the pitch and yaw rotations into
    //the final rotation matrix
    quat pitchRotation = glm::angleAxis(pitch, vec3(1.f, 0.f, 0.f));
    quat yawRotation = glm::angleAxis(yaw, vec3( 0.f, -1.f, 0.f));

    return glm::mat4(yawRotation) * glm::mat4(pitchRotation);
}