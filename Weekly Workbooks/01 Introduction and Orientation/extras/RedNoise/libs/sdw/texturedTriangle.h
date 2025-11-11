#pragma once

#include "DrawingWindow.h"
#include "camera.h"
#include "face.h"

void triangleTextured3D(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], Face face, cameraClass &camera);