#include <algorithm>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>
#include "DrawingWindow.h"
#include "Colour.h"
#include "CanvasPoint.h"
#include "Utils.h"
#include "RayTriangleIntersection.h"
#include "face.h"
#include "raytraceUtils.h"

#include "photonMap.h"

// scatter photon in random direction of semi-sphere when it hits a diffuse surface
glm::vec3 randomDiffuse(glm::vec3 normal) {
	float z = getRand(); 
	float t = 2.0f * M_PI * getRand();
	float r = sqrt(1 - z*z);
	glm::vec3 direction = glm::normalize(glm::vec3(r * cos(t), r * sin(t), z));
	glm::vec3 u = glm::normalize(glm::cross(glm::vec3(0,1,0), normal));
	glm::vec3 v = glm::cross(normal, u);
	return glm::normalize(u*direction.x + v*direction.y + normal*direction.z);
}

// recursive function to trace a photon through scene 
void tracePhoton(std::vector<Face> &model, std::vector<photon> &output, glm::vec3 origin, glm::vec3 direction, glm::vec3 power, int recursionDepth) {
	// find surface of intersection
	RayTriangleIntersection intersection = getClosestIntersection(model, origin, direction, -1);
	Colour colour;
	photon p;
	if (recursionDepth <= 0) return;
	else if ((intersection.triangleIndex >= 0) && (intersection.distanceFromCamera != std::numeric_limits<float>::infinity())) {
		Face face = model[intersection.triangleIndex];
		glm::vec3 location = glm::vec3(intersection.intersectionPoint);
		glm::vec3 normal = face.getNormal(intersection, model);
		if (face.opacity < 1.0f) { //Material transparent
			glm::vec3 refracted;
			float ratio;
			if (glm::dot(direction, normal) < 0.0f) { //Ray entering
				ratio = (1.0f/1.5f);
				refracted = glm::refract(direction, normal, ratio); 
			} else { //Ray exiting 
				normal = -normal;
				ratio = (1.5f/1.0f);
				refracted = glm::refract(direction, normal, ratio); 
			}
			if (glm::length(refracted) < 1e-6f) { //TIR
				glm::vec3 R = glm::normalize(glm::reflect(direction, normal));
				tracePhoton(model, output, intersection.intersectionPoint, R, power, recursionDepth-1); 
			} else { //Refraction
				refracted = glm::normalize(refracted);
				// weight by transmission colour of material - gives nice coloured caustics
				glm::vec3 newPower = power * face.transmission;
				tracePhoton(model, output, intersection.intersectionPoint, refracted, newPower, recursionDepth-1);
			}
		} else {
			if (face.mirror) { //Mirror, reflect all
				glm::vec3 R = glm::normalize(glm::reflect(direction, normal));
				R = glm::normalize(R + glm::vec3(2*getRand()-1, 2*getRand()-1, 2*getRand()-1) * face.fuzziness);
				glm::vec3 newPower = power * face.diffuse;
				tracePhoton(model, output, intersection.intersectionPoint, R, newPower, recursionDepth-1);
			} else { //Other surface
				glm::vec3 newPower = power * face.diffuse;
				p.location = location;
				p.power = newPower;
				p.incidence = direction;
				output.push_back(p);
				// "russian roulette" to determine if photon is also reflected
				// probability is based on apparent brightness of surface 
				float prob = fmax(face.diffuse[0], fmax(face.diffuse[1], face.diffuse[2]));
				float roulette = (getRand() < prob);
				if (roulette) {
					// reflect in random direction
					glm::vec3 direction = randomDiffuse(normal);
					tracePhoton(model, output, intersection.intersectionPoint, direction, newPower/prob, recursionDepth-1);
				}
			}
		}
	}
}

std::vector<photon> photonMap(std::vector<Face> &model, int count, cameraClass &camera) {
	std::vector<photon> map;
	photon point;
	// shoot photons randomly in all directions 
	for (int i=0; i<count; i++) {
		float z = 2.0f * getRand() - 1.0f;
		float t = 2.0f * M_PI * getRand();
		float r = sqrt(1 - z*z);
		glm::vec3 direction = glm::vec3(r * cos(t), r * sin(t), z);
		float power = 1.0f;
		tracePhoton(model, map, camera.light, direction, glm::vec3(power/count), 6);
		if (i%1000 == 0) std::cout<<(float)i/(float)count<<std::endl;
	}
	return map;
}