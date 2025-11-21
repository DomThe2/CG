#pragma once

#include <vector>
#include "TextureMap.h"
#include "CanvasPoint.h"
 
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
		float viewportHeight; 
		float viewportWidth;
		bool orbit;
		TextureMap environment;

		CanvasPoint projectVertexOntoCanvasPoint(glm::vec3 vertexPosition) {
			glm::vec3 relativePosition = glm::transpose(cameraOri) * (vertexPosition - cameraPos);
			float x = std::round((focalLength * (relativePosition.x / relativePosition.z)) * (WIDTH/2.0f) + (WIDTH / 2.0f));	
			float y = std::round((focalLength * (relativePosition.y / relativePosition.z)) * (WIDTH/2.0f) + (HEIGHT / 2.0f));
			float depth = -1.0/relativePosition.z;
			x = WIDTH - x;
			return CanvasPoint(x, y, depth);
		} 

		void handleEvent(SDL_Event event, DrawingWindow &window) {
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_o) orbit = !orbit;
				else if (event.key.keysym.sym == SDLK_a) cameraPos.x -= 0.05f;
				else if (event.key.keysym.sym == SDLK_d) cameraPos.x += 0.05f;
				else if (event.key.keysym.sym == SDLK_w) cameraPos.y+= 0.05f;
				else if (event.key.keysym.sym == SDLK_s) cameraPos.y -= 0.05f;
				else if (event.key.keysym.sym == SDLK_z) cameraPos.z -= 0.05f;
				else if (event.key.keysym.sym == SDLK_x) cameraPos.z += 0.05f;
				else if (event.key.keysym.sym == SDLK_h) cameraPos = xMatrix(0.01, -1) * cameraPos;
				else if (event.key.keysym.sym == SDLK_k) cameraPos = xMatrix(0.01, 1) * cameraPos;
				else if (event.key.keysym.sym == SDLK_u) cameraPos = yMatrix(0.01, -1) * cameraPos;
				else if (event.key.keysym.sym == SDLK_j) cameraPos = yMatrix(0.01, 1) * cameraPos; 
				else if (event.key.keysym.sym == SDLK_UP) cameraOri = yMatrix(0.01, -1) * cameraOri;
				else if (event.key.keysym.sym == SDLK_DOWN) cameraOri = yMatrix(0.01, 1) * cameraOri;
				else if (event.key.keysym.sym == SDLK_LEFT) cameraOri = xMatrix(0.01, -1) * cameraOri;
				else if (event.key.keysym.sym == SDLK_RIGHT) cameraOri = xMatrix(0.01, 1) * cameraOri;
				else if (event.key.keysym.sym == SDLK_RETURN) toggleMode();
			} else if (event.type == SDL_MOUSEBUTTONDOWN) { 
				window.savePPM("output.ppm");
				window.saveBMP("output.bmp");
			}
		}

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


