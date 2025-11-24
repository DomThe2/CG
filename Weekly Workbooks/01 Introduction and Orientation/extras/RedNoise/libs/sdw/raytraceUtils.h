#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "RayTriangleIntersection.h"
#include "face.h"

RayTriangleIntersection getClosestIntersection(std::vector<Face>& model, glm::vec3 cameraPosition, glm::vec3 rayDirection, int ignoreIndex);

