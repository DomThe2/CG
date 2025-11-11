#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "camera.h"
#include "CanvasPoint.h"

std::vector<std::string> split(const std::string &line, char delimiter);
glm::vec3 convertToBarycentricCoordinates(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, glm::vec2 r);
glm::vec3 convertToBarycentricCoordinates(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 r);
std::vector<float> interpolateSingleFloats(float from, float to, float numberOfValues);
CanvasPoint projectVertexOntoCanvasPoint(cameraClass &camera, glm::vec3 vertexPosition);
glm::mat3 xMatrix(float angle, float direction);
glm::mat3 yMatrix(float angle, float direction);
void lookAt(glm::vec3 location, cameraClass &camera);

