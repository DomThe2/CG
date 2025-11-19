#include <algorithm>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"

#include "CanvasPoint.h"
#include "camera.h"
#include "Utils.h"

void lookAt(glm::vec3 location, cameraClass &camera) {
	glm::vec3 forward = glm::normalize(camera.cameraPos - location);
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), forward));
	glm::vec3 up = glm::cross(forward, right);
	camera.cameraOri = glm::mat3(right, up, forward);
	
}

CanvasPoint projectVertexOntoCanvasPoint(cameraClass &camera, glm::vec3 vertexPosition) {
	glm::vec3 relativePosition = glm::transpose(camera.cameraOri) * (vertexPosition - camera.cameraPos);
    float x = std::round((camera.focalLength * (relativePosition.x / relativePosition.z)) * (WIDTH/2.0f) + (WIDTH / 2.0f));	
    float y = std::round((camera.focalLength * (relativePosition.y / relativePosition.z)) * (WIDTH/2.0f) + (HEIGHT / 2.0f));
	float depth = -1.0/relativePosition.z;
	x = WIDTH - x;
	return CanvasPoint(x, y, depth);
} 

void handleEvent(SDL_Event event, DrawingWindow &window, cameraClass &camera) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_o) camera.orbit = !camera.orbit;
		else if (event.key.keysym.sym == SDLK_a) camera.cameraPos.x -= 0.05f;
		else if (event.key.keysym.sym == SDLK_d) camera.cameraPos.x += 0.05f;
		else if (event.key.keysym.sym == SDLK_w) camera.cameraPos.y+= 0.05f;
		else if (event.key.keysym.sym == SDLK_s) camera.cameraPos.y -= 0.05f;
		else if (event.key.keysym.sym == SDLK_z) camera.cameraPos.z -= 0.05f;
		else if (event.key.keysym.sym == SDLK_x) camera.cameraPos.z += 0.05f;
		else if (event.key.keysym.sym == SDLK_h) camera.cameraPos = xMatrix(0.01, -1) * camera.cameraPos;
		else if (event.key.keysym.sym == SDLK_k) camera.cameraPos = xMatrix(0.01, 1) * camera.cameraPos;
		else if (event.key.keysym.sym == SDLK_u) camera.cameraPos = yMatrix(0.01, -1) * camera.cameraPos;
		else if (event.key.keysym.sym == SDLK_j) camera.cameraPos = yMatrix(0.01, 1) * camera.cameraPos; 
		else if (event.key.keysym.sym == SDLK_UP) camera.cameraOri = yMatrix(0.01, -1) * camera.cameraOri;
		else if (event.key.keysym.sym == SDLK_DOWN) camera.cameraOri = yMatrix(0.01, 1) * camera.cameraOri;
    	else if (event.key.keysym.sym == SDLK_LEFT) camera.cameraOri = xMatrix(0.01, -1) * camera.cameraOri;
		else if (event.key.keysym.sym == SDLK_RIGHT) camera.cameraOri = xMatrix(0.01, 1) * camera.cameraOri;
		else if (event.key.keysym.sym == SDLK_RETURN) camera.toggleMode();
	} else if (event.type == SDL_MOUSEBUTTONDOWN) { 
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}
