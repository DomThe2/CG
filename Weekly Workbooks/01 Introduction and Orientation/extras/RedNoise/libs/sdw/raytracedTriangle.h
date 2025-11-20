#pragma once

#include "DrawingWindow.h"
#include "camera.h"
#include "face.h"
#include "kdTree.h"
#include "Utils.h"

void drawRaytraced(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera); 
std::vector<photon> photonMap(std::vector<Face> &model, int count, cameraClass &camera);