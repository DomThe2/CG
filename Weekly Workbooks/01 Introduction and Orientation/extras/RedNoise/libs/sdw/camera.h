#pragma once

#include <vector>
#include "TextureMap.h"
 
#define WIDTH 320
#define HEIGHT 240

class cameraClass {
	public:
		glm::vec3 cameraPos;
		glm::mat3 cameraOri;
		glm::vec3 light;
		std::string mode;
		float focalLength;
		bool orbit;
		TextureMap environment;

		void toggleMode() {
			if (mode == "wireframe") {
				mode = "rasterised";
			} else if (mode == "rasterised") {
				mode = "raytraced";
			} else {
				mode = "wireframe";
			}
		}
};


