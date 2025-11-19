#pragma once

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "CanvasPoint.h"

std::vector<Face> loadTriangle(std::string file, float scale);
void storePhotonMap(std::vector<photon> lightMap, std::string filename);
std::vector<photon> getPhotonMap(std::string filename);

 