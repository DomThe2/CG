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

// determine if pixel is shadowed 
float inShadow(std::vector<Face> &model, RayTriangleIntersection ray, glm::vec3 light, int shellNumber) {
	float radius = 0.05f/shellNumber;
	float w = 0;
	float n = 0;
	// increment ray origin in disk around vector pointing from light to point 
	// produces soft shadows 
	glm::vec3 direction = glm::normalize(light - ray.intersectionPoint);
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), direction));
	glm::vec3 up = glm::cross(direction, right);
	for (int i=1; i<=shellNumber; i++) {
		for (int j=0; j<(2*i*i); j++) {
			// determine point in circle on disk
			float a = (2 * M_PI * j) / (2*i*i);
			float x = (i * radius) * std::cos(a);
			float y = (i * radius) * std::sin(a); 
			glm::vec3 offset = x * right + y * up;
			glm::vec3 relativePos = (light + offset) - ray.intersectionPoint;	
			float distance = glm::length(relativePos);
			// determine if another object is between point and light source 
			RayTriangleIntersection closest = getClosestIntersection(model, ray.intersectionPoint, glm::normalize(relativePos), ray.triangleIndex);
			bool occluded = (closest.distanceFromCamera < distance) && (closest.distanceFromCamera != std::numeric_limits<float>::infinity());
			if (!occluded) w++;
			n++;
		} 
	}
	// return proportion of rays from disk to light that have been blocked
	return w/n;
}

// get environment map 
Colour getEnvironment(glm::vec3 ray, cameraClass &camera) {
	// convert into spherical polar coordinates 
	float rho = sqrt(ray.x*ray.x + ray.y*ray.y + ray.z*ray.z);
	float theta  = atan2(ray.z, ray.x);
	float phi = acosf(ray.y/rho);
	// determine x, y coordinates of enviroment map 
	int x = ((theta + M_PI) / (2 * M_PI)) * camera.environment.width;
	int y = (phi / M_PI) * camera.environment.height;
	// clamp into environment map dimensions 
	x = x % (int)camera.environment.width;
	y = glm::clamp(y, 0, (int)camera.environment.height-1);
	// get colour at environment map coordinates
    uint32_t raw = camera.environment.pixels[y * camera.environment.width + x];
    int r = (raw >> 16) & 0xFF;
    int g = (raw >> 8) & 0xFF;
    int b = raw & 0xFF;
    return Colour(r, g, b);
}

// search the kd tree for the n nearest photons 
glm::vec3 searchkdTreeForN(node* kdTree, RayTriangleIntersection intersection, glm::vec3 normal, int N) {
	std::priority_queue<photon> nearby;
	searchkdTree(kdTree, nearby, intersection.intersectionPoint, N);
	// determine area photons are contained in 
	float radius = nearby.top().distance;
	float area = M_PI * radius * radius;
	glm::vec3 energy = glm::vec3(0,0,0);
	while(!nearby.empty()) {
		// weight by their angle of incidence to the normal
		glm::vec3 w = glm::normalize(-nearby.top().incidence);
		float angle = std::fmax(glm::dot(w, normal), 0.0f);
		// weight by their power 
		energy += nearby.top().power * angle;
		nearby.pop();
	}
	// weight by the inverse of the area the photons are spread over
	return energy / (float)(M_PI * area);
}

// find specular value of point 
float specularValue(RayTriangleIntersection intersection, glm::vec3 normal, float coarseness, cameraClass &camera) {
    glm::vec3 Ri = glm::normalize(intersection.intersectionPoint - camera.light);
    glm::vec3 N = glm::normalize(normal);
    glm::vec3 Rr = glm::reflect(Ri, N);
    glm::vec3 V = glm::normalize(camera.cameraPos - intersection.intersectionPoint);
    float dot = glm::dot(V, Rr);
    if (dot < 0.0f) return 0.01f;
    return std::fmax(pow(dot, coarseness), 0.0f);
}

// get lighting of point using all lighting techniques
glm::vec3 getLighting(std::vector<Face> &model, node* kdTree, glm::vec3 colour, glm::vec3 normal, RayTriangleIntersection intersection, cameraClass &camera) {
	Face face = model[intersection.triangleIndex];
	// gather various light values 
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
	// return sum weighted by camera "exposure", simulating camera sensor sensitivity 
	return glm::min((ambientTerm + diffuseTerm + specularTerm + photonTerm) * camera.exposure, glm::vec3(1.0f));
}

//Forward declaration to allow mutual recursion
Colour getColour(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, Face face, cameraClass &camera, int recursionDepth);

Colour getReflective(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, cameraClass &camera, int recursionDepth) {
	Face face = model[intersection.triangleIndex];
	Colour colour;
	// reflect ray off surface 
	glm::vec3 R = glm::reflect(ray, normal);
	R = glm::normalize(R + glm::vec3(2*getRand()-1, 2*getRand()-1, 2*getRand()-1) * face.fuzziness);
	RayTriangleIntersection reflected = getClosestIntersection(model, intersection.intersectionPoint, R, intersection.triangleIndex);
	if (reflected.distanceFromCamera != std::numeric_limits<float>::infinity()) {
		// if it hits an object recur 
		glm::vec3 reflectedNormal = face.getNormal(reflected, model);
		colour = getColour(model, kdTree, reflected, R, reflectedNormal, model[reflected.triangleIndex], camera, recursionDepth - 1) * face.diffuse;
	} else {
		// otherwise get the environment colour 
		colour = getEnvironment(R, camera) * face.diffuse;
	}
	// also add specular and shadow values to reflective surfaces
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
	// refract ray
	if (glm::dot(ray, N) < 0.0f) { //Ray entering
		ratio = (1.0f/1.5f); // Assume air->glass
		refracted = glm::refract(ray, N, ratio); 
	} else { //Ray exiting 
		N = -N;
		ratio = (1.5f/1.0f); // Assume glass->air
		refracted = glm::refract(ray, N, ratio); 
	}
	if (glm::length(refracted) < 1e-6f) {  // TIR, treat as mirror
		glm::vec3 r = glm::normalize(glm::reflect(ray, N));
		RayTriangleIntersection newIntersection = getClosestIntersection(model, intersection.intersectionPoint, r, intersection.triangleIndex);
		if (newIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
			// if hits an object, recur 
			glm::vec3 newIntersectionNormal = face.getNormal(newIntersection, model);
			colour = getColour(model, kdTree, newIntersection, r, newIntersectionNormal, model[newIntersection.triangleIndex], camera, recursionDepth-1);
		} else {
			// otherwise get environment map 
			colour = getEnvironment(r, camera);
		}
	} else { // Refraction
		refracted = glm::normalize(refracted);
		RayTriangleIntersection newIntersection = getClosestIntersection(model, intersection.intersectionPoint, refracted, intersection.triangleIndex);
		if (newIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {	
			// if hits and object, recur 
			glm::vec3 newIntersectionNormal = face.getNormal(newIntersection, model);
			colour = getColour(model, kdTree, newIntersection, refracted, newIntersectionNormal, model[newIntersection.triangleIndex], camera, recursionDepth-1) * face.transmission;
		} else {
			// otherwise get environment map 
			colour = getEnvironment(refracted, camera) * face.transmission;
		}
	}
	// also add specular and shadow values to refractive surfaces
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
		// otherwise standard surface, get colour 
		glm::vec3 colour;
		if (face.textured) {
			// get texture colour 
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
		// get lighting of point
		glm::vec3 total = getLighting(model, kdTree, colour, normal, intersection, camera);
		Colour output = Colour(total[0]*255.0f, total[1]*255.0f, total[2]*255.0f);
		return (output);
	}
}

// set focal distance to object camera is currently looking at
void focusCamera(std::vector<Face> &model, cameraClass &camera) {
	// shoot ray directly out of center of camera into scene 
	glm::vec3 ray = camera.cameraOri * glm::vec3(0, 0, -1);
	RayTriangleIntersection intersection = getClosestIntersection(model, camera.cameraPos, ray, -1);
	if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
		camera.focalDistance = intersection.distanceFromCamera;
	}
}

// find vector from camera ray towards pixel of screen 
glm::vec3 cameraRay(std::vector<Face> &model, float i, float j, glm::vec3 origin, cameraClass &camera) {
	// find coordinates of screen pixel
	float viewportX = -camera.viewportWidth/2.0f + j * camera.viewportWidth/WIDTH;
	float viewportY = camera.viewportHeight/2.0f + i * -camera.viewportHeight/HEIGHT;
	glm::vec3 directionCameraSpace = glm::vec3(viewportX, viewportY, -camera.focalLength);
	// apply camera position and orientation
	glm::vec3 directionWorldSpace = camera.cameraOri * directionCameraSpace;
	float distanceToFocalPlane = camera.focalDistance / camera.focalLength;
	glm::vec3 focalPoint = camera.cameraPos + directionWorldSpace * distanceToFocalPlane;
	glm::vec3 rayDirection = glm::normalize(focalPoint - origin);
	return rayDirection;
}

// main loop of thread
void mainLoop(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera, int start, int end) {
	SDL_Event event;
	for (float i=start; i<end; i++) {
		// poll for events to prevent hang when drawing
		if (window.pollForInputEvents(event)) camera.handleEvent(event, window);
		for (float j=0; j<WIDTH; j++) {	
			Colour colour;
			// take multiple samples for bokeh effect 
			for (int k=0; k<camera.dofSamples; k++) {
				// get camera location - randomised in a disk around actual location
				glm::vec3 origin = camera.getCameraLocation();
				glm::vec3 rayDirection = cameraRay(model, i, j, origin, camera);
				RayTriangleIntersection intersection = getClosestIntersection(model, origin, rayDirection, -1);
				if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
					// if hits something, get colour using recursive colour function
					glm::vec3 normal = glm::normalize(model[intersection.triangleIndex].getNormal(intersection, model));
					colour = colour + getColour(model, kdTree, intersection, rayDirection, normal, model[intersection.triangleIndex], camera, 6);
				} else {
					// otherwise get environment colour 
					colour = colour + getEnvironment(rayDirection, camera);
				}
			}
			// average colour over all samples
			colour = colour * (1.0f/(float)camera.dofSamples);
			uint32_t pixelColour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
			window.setPixelColour(j, i, pixelColour);
		}
	}
}

void drawRaytraced(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera, int threads) {
	// reset random seed each frame to make randomised effects stable between frames
	srand(0);
	// orbit 
	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		camera.lookAt(glm::vec3(0,0,0));
	}
	// launch threads
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
		// if only one thread used, run without threading
		mainLoop(window, model, kdTree, camera, 0, HEIGHT);
	}
	window.renderFrame();
}