#pragma once

#include "DrawingWindow.h"
#include "camera.h"
#include "face.h"
#include "kdTree.h"
#include "Utils.h"

void focusCamera(std::vector<Face> &model, cameraClass &camera);
void drawRaytraced(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera, int threads); 
std::vector<photon> photonMap(std::vector<Face> &model, int count, cameraClass &camera);
void getVertexColours(std::vector<Face> &model, node* kdTree, cameraClass &camera);