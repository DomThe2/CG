#pragma once

#include "DrawingWindow.h"
#include "camera.h"

void triangle3D(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], ModelTriangle triangle, cameraClass &camera); 
void drawLine(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasPoint from, CanvasPoint to, Colour colour);