#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "camera.h"
#include "CanvasPoint.h"

void handleEvent(SDL_Event event, DrawingWindow &window, cameraClass &camera);
CanvasPoint projectVertexOntoCanvasPoint(cameraClass &camera, glm::vec3 vertexPosition);
void lookAt(glm::vec3 location, cameraClass &camera);

