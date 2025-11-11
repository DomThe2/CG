#pragma once

#include <glm/glm.hpp>

#define WIDTH 320
#define HEIGHT 240

class cameraClass {
	public:
		glm::vec3 cameraPos;
		glm::mat3 cameraOri;
		glm::vec3 light;
		float focalLength;
		bool orbit;
		bool phong;
		std::string mode;

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


