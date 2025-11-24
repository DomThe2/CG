#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <math.h>
#include <queue>
#include <thread>

#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "CanvasTriangle.h"
#include "ModelTriangle.h"
#include "Colour.h"
#include "Utils.h"
#include "camera.h"
#include "RayTriangleIntersection.h"
#include "face.h"
#include "kdTree.h"
#include "raytraceUtils.h"

#include "raytracedTriangle.h"

//TODO:
// - improve metallic surfaces 
// - build render scene - hall of materials
// - write render track
// - add comments
// need 25 * 15 = 375 frames

float inShadow(std::vector<Face> &model, RayTriangleIntersection ray, glm::vec3 light, int shellNumber) {
	float radius = 0.05f/shellNumber;
	float w = 0;
	float n = 0;
	glm::vec3 direction = glm::normalize(light - ray.intersectionPoint);
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), direction));
	glm::vec3 up = glm::cross(direction, right);
	for (int i=1; i<=shellNumber; i++) {
		for (int j=0; j<(2*i*i); j++) {
			float a = (2 * M_PI * j) / (2*i*i);
			float x = (i * radius) * std::cos(a);
			float y = (i * radius) * std::sin(a); 
			glm::vec3 offset = x * right + y * up;
			glm::vec3 relativePos = (light + offset) - ray.intersectionPoint;	
			float distance = glm::length(relativePos);
			RayTriangleIntersection closest = getClosestIntersection(model, ray.intersectionPoint, glm::normalize(relativePos), ray.triangleIndex);
			bool occluded = (closest.distanceFromCamera < distance) && (closest.distanceFromCamera != std::numeric_limits<float>::infinity());
			if (!occluded) w++;
			n++;
		} 
	}
	return w/n;
}

Colour getEnvironment(glm::vec3 ray, cameraClass &camera) {
	float rho = sqrt(ray.x*ray.x + ray.y*ray.y + ray.z*ray.z);
	float theta  = atan2(ray.z, ray.x);
	float phi = acosf(ray.y/rho);
	int x = ((theta + M_PI) / (2 * M_PI)) * camera.environment.width;
	int y = (phi / M_PI) * camera.environment.height;
	x = x % (int)camera.environment.width;
	y = glm::clamp(y, 0, (int)camera.environment.height-1);
    uint32_t raw = camera.environment.pixels[y * camera.environment.width + x];
    int r = (raw >> 16) & 0xFF;
    int g = (raw >> 8) & 0xFF;
    int b = raw & 0xFF;
    return Colour(r, g, b);
}

glm::vec3 searchkdTreeForN(node* kdTree, RayTriangleIntersection intersection, glm::vec3 normal, int N) {
	std::priority_queue<photon> nearby;
	searchkdTree(kdTree, nearby, intersection.intersectionPoint, N);
	float radius = nearby.top().distance;
	float area = M_PI * radius * radius;
	glm::vec3 energy = glm::vec3(0,0,0);
	while(!nearby.empty()) {
		glm::vec3 w = glm::normalize(-nearby.top().incidence);
		float angle = std::fmax(glm::dot(w, normal), 0.0f);
		energy += nearby.top().power * angle;
		nearby.pop();
	}
	return energy / (float)(M_PI * area);
}

float specularValue(RayTriangleIntersection intersection, glm::vec3 normal, float coarseness, cameraClass &camera) {
    glm::vec3 Ri = glm::normalize(intersection.intersectionPoint - camera.light);
    glm::vec3 N = glm::normalize(normal);
    glm::vec3 Rr = glm::reflect(Ri, N);
    glm::vec3 V = glm::normalize(camera.cameraPos - intersection.intersectionPoint);
    float dot = glm::dot(V, Rr);
    if (dot < 0.0f) return 0.01f;
    return std::fmax(pow(dot, coarseness), 0.0f);
}

glm::vec3 getLighting(std::vector<Face> &model, node* kdTree, glm::vec3 colour, glm::vec3 normal, RayTriangleIntersection intersection, cameraClass &camera) {
	Face face = model[intersection.triangleIndex];
	float ambient = 0.0f; 
	float distance = glm::length(camera.light - intersection.intersectionPoint);
	float proximity = 1.0f / (distance * distance + 1.0f); 
	float angle = std::fmax(glm::dot(glm::normalize(camera.light - intersection.intersectionPoint), normal), 0.0f);
	float specular = specularValue(intersection, normal, face.specularExponent, camera);
	float shadow = (0.2f + inShadow(model, intersection, camera.light, camera.shadowSamples));
	glm::vec3 photonTerm = searchkdTreeForN(kdTree, intersection, normal, 1000) * 5.0f + searchkdTreeForN(kdTree, intersection, normal, 50);
	glm::vec3 ambientTerm = face.ambient * ambient;
	glm::vec3 diffuseTerm = colour * angle * proximity * shadow;
	glm::vec3 specularTerm = face.specular * specular * shadow;
	return glm::min((ambientTerm + diffuseTerm + specularTerm + photonTerm) * camera.exposure, glm::vec3(1.0f));
}

//Forward declaration to allow mutual recursion
Colour getColour(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, Face face, cameraClass &camera, int recursionDepth);

Colour getReflective(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, cameraClass &camera, int recursionDepth) {
	Face face = model[intersection.triangleIndex];
	Colour colour;
	glm::vec3 R = glm::reflect(ray, normal);
	R = glm::normalize(R + glm::vec3(2*getRand()-1, 2*getRand()-1, 2*getRand()-1) * face.fuzziness);
	RayTriangleIntersection reflected = getClosestIntersection(model, intersection.intersectionPoint, R, intersection.triangleIndex);
	if (reflected.distanceFromCamera != std::numeric_limits<float>::infinity()) {
		glm::vec3 reflectedNormal = face.getNormal(reflected, model);
		colour = getColour(model, kdTree, reflected, R, reflectedNormal, model[reflected.triangleIndex], camera, recursionDepth - 1) * face.diffuse;
	} else {
		colour = getEnvironment(R, camera) * face.diffuse;
	}
	float specular = specularValue(intersection, normal, face.specularExponent, camera);
	float shadow = (0.2f + inShadow(model, intersection, camera.light, camera.shadowSamples));
	glm::vec3 specularTerm = face.specular * specular * shadow;
	glm::vec3 output = glm::min(glm::vec3(colour.red, colour.green, colour.blue) + specularTerm * 255.0f, glm::vec3(255.0f));
	return Colour(output[0], output[1], output[2]);
}

Colour getTransparent(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, cameraClass &camera, int recursionDepth) {
	Face face = model[intersection.triangleIndex];
	Colour colour;
	glm::vec3 refracted;
	glm::vec3 N = normal;
	float ratio;
	if (glm::dot(ray, N) < 0.0f) { //Ray entering
		ratio = (1.0f/1.5f); // Assume air->glass
		refracted = glm::refract(ray, N, ratio); 
	} else { //Ray exiting 
		N = -N;
		ratio = (1.5f/1.0f); // Assume glass->air
		refracted = glm::refract(ray, N, ratio); 
	}
	if (glm::length(refracted) < 1e-6f) {  // TIR
		glm::vec3 r = glm::normalize(glm::reflect(ray, N));
		RayTriangleIntersection newIntersection = getClosestIntersection(model, intersection.intersectionPoint, r, intersection.triangleIndex);
		if (newIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
			glm::vec3 newIntersectionNormal = face.getNormal(newIntersection, model);
			colour = getColour(model, kdTree, newIntersection, r, newIntersectionNormal, model[newIntersection.triangleIndex], camera, recursionDepth-1);
		} else {
			colour = getEnvironment(r, camera);
		}
	} else { // Refraction
		refracted = glm::normalize(refracted);
		RayTriangleIntersection newIntersection = getClosestIntersection(model, intersection.intersectionPoint, refracted, intersection.triangleIndex);
		if (newIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {	
			glm::vec3 newIntersectionNormal = face.getNormal(newIntersection, model);
			colour = getColour(model, kdTree, newIntersection, refracted, newIntersectionNormal, model[newIntersection.triangleIndex], camera, recursionDepth-1) * face.transmission;
		} else {
			colour = getEnvironment(refracted, camera) * face.transmission;
		}
	}
	float specular = specularValue(intersection, normal, face.specularExponent, camera);
	float shadow = (0.2f + inShadow(model, intersection, camera.light, camera.shadowSamples));
	glm::vec3 specularTerm = face.specular * specular * shadow;
	glm::vec3 output = glm::min(glm::vec3(colour.red, colour.green, colour.blue) + specularTerm * 255.0f, glm::vec3(255.0f));
	return Colour(output[0], output[1], output[2]);
}

Colour getColour(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, Face face, cameraClass &camera, int recursionDepth) {
	if (intersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
		return getEnvironment(ray, camera);
    } else if (face.mirror && (recursionDepth > 0)) { //Reflective surface 
		return getReflective(model, kdTree, intersection, ray, normal, camera, recursionDepth);
	} else if ((face.opacity<1.0f) && (recursionDepth > 0)) { // Transparent surface
		return getTransparent(model, kdTree, intersection, ray, normal, camera, recursionDepth);
	} else {
		glm::vec3 colour;
		if (face.textured) {
			glm::vec3 barycentric = face.getBarycentricOfIntersection(intersection);
			float u = barycentric.z * face.triangle.texturePoints[0].x + barycentric.x * face.triangle.texturePoints[1].x + barycentric.y * face.triangle.texturePoints[2].x;
			float v = barycentric.z * face.triangle.texturePoints[0].y + barycentric.x * face.triangle.texturePoints[1].y + barycentric.y * face.triangle.texturePoints[2].y;
			uint32_t raw = face.texture.pixels[(int)v * face.texture.width + (int)u];
			float r = ((raw >> 16) & 0xFF)/255.0f;
			float g = ((raw >> 8) & 0xFF)/255.0f;
			float b = (raw & 0xFF)/255.0f;
			colour = glm::vec3(r,g,b);
		} else {
			colour = face.diffuse;
		}
		glm::vec3 total = getLighting(model, kdTree, colour, normal, intersection, camera);
		Colour output = Colour(total[0]*255.0f, total[1]*255.0f, total[2]*255.0f);
		return (output);
	}
}

void focusCamera(std::vector<Face> &model, cameraClass &camera) {
	glm::vec3 ray = camera.cameraOri * glm::vec3(0, 0, -1);
	RayTriangleIntersection intersection = getClosestIntersection(model, camera.cameraPos, ray, -1);
	if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
		camera.focalDistance = intersection.distanceFromCamera;
	}
}

glm::vec3 cameraRay(std::vector<Face> &model, float i, float j, glm::vec3 origin, cameraClass &camera) {
	float viewportX = -camera.viewportWidth/2.0f + j * camera.viewportWidth/WIDTH;
	float viewportY = camera.viewportHeight/2.0f + i * -camera.viewportHeight/HEIGHT;
	glm::vec3 directionCameraSpace = glm::vec3(viewportX, viewportY, -camera.focalLength);
	glm::vec3 directionWorldSpace = camera.cameraOri * directionCameraSpace;
	float distanceToFocalPlane = camera.focalDistance / camera.focalLength;
	glm::vec3 focalPoint = camera.cameraPos + directionWorldSpace * distanceToFocalPlane;
	glm::vec3 rayDirection = glm::normalize(focalPoint - origin);
	return rayDirection;
}

void mainLoop(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera, int start, int end) {
	SDL_Event event;
	for (float i=start; i<end; i++) {
		if (window.pollForInputEvents(event)) camera.handleEvent(event, window);
		for (float j=0; j<WIDTH; j++) {	
			Colour colour;
			for (int k=0; k<camera.dofSamples; k++) {
				glm::vec3 origin = camera.getCameraLocation();
				glm::vec3 rayDirection = cameraRay(model, i, j, origin, camera);
				RayTriangleIntersection intersection = getClosestIntersection(model, origin, rayDirection, -1);
				if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
					glm::vec3 normal = glm::normalize(model[intersection.triangleIndex].getNormal(intersection, model));
					colour = colour + getColour(model, kdTree, intersection, rayDirection, normal, model[intersection.triangleIndex], camera, 6);
				} else {
					colour = colour + getEnvironment(rayDirection, camera);
				}
			}
			colour = colour * (1.0f/(float)camera.dofSamples);
			uint32_t pixelColour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
			window.setPixelColour(j, i, pixelColour);
		}
	}
}

void drawRaytraced(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera, int threads) {
	/*
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}
		*/
	srand(0);
	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		camera.lookAt(glm::vec3(0,0,0));
	}
	if (threads > 1) {
		std::vector<std::thread> threadVector;
		int increment = HEIGHT/threads;
		int start = 0;
		int end = increment;
		for (int i=0; i<threads; i++) {
			threadVector.emplace_back(mainLoop, std::ref(window), std::ref(model), kdTree, std::ref(camera), start, end);
			start += increment;
			end += increment;
		}
		for (int i=0; i<threads; i++) {
			threadVector[i].join();
		}
	} else {
		mainLoop(window, model, kdTree, camera, 0, HEIGHT);
	}
	window.renderFrame();
}