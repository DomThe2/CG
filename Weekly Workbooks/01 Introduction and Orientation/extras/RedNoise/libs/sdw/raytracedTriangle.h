#pragma once

#include "DrawingWindow.h"
#include "camera.h"

void drawRaytraced(DrawingWindow &window,  std::vector<Face> model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera); 