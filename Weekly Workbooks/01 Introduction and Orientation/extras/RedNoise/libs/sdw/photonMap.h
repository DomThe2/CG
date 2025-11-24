#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "RayTriangleIntersection.h"
#include "face.h"
#include "camera.h"

std::vector<photon> photonMap(std::vector<Face> &model, int count, cameraClass &camera);
