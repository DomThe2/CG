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
		float focalDistance;
		float lensRadius;
		bool orbit;
		TextureMap environment;

		glm::vec3 getCameraLocation() {
			float theta = 2 * M_PI * (float(rand())/(float)RAND_MAX);
			float r = lensRadius * sqrt((float(rand())/(float)RAND_MAX));
			float x = r * cos(theta);
			float y = r * sin(theta);
			glm::vec3 right = cameraOri[0];
			glm::vec3 up = cameraOri[1];
			return right * x + up * y + cameraPos;
		}

		void lookAt(glm::vec3 location) {
			glm::vec3 forward = glm::normalize(cameraPos - location);
			glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), forward));
			glm::vec3 up = glm::cross(forward, right);
			cameraOri = glm::mat3(right, up, forward);
		}

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


