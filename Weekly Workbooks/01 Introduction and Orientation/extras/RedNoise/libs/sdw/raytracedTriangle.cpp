#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <math.h>
#include <queue>

#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "CanvasTriangle.h"
#include "ModelTriangle.h"
#include "Colour.h"
#include "Utils.h"
#include "cameraUtils.h"
#include "camera.h"
#include "RayTriangleIntersection.h"
#include "face.h"
#include "kdTree.h"

#include "raytracedTriangle.h"

//TODO:
// - add diffuse scattering

glm::vec3 getBarycentricOfIntersection(RayTriangleIntersection intersection) {
	return convertToBarycentricCoordinates(intersection.intersectedTriangle.vertices[0], intersection.intersectedTriangle.vertices[1], 
        	intersection.intersectedTriangle.vertices[2], intersection.intersectionPoint);
}

RayTriangleIntersection getClosestIntersection(std::vector<Face>& model, glm::vec3 cameraPosition, glm::vec3 rayDirection, int ignoreIndex) {
	RayTriangleIntersection intersection;
	intersection.distanceFromCamera = std::numeric_limits<float>::infinity();
	intersection.triangleIndex = -1;
	for (int i=0; i<model.size(); i++) {
		glm::vec3 e0 = model[i].triangle.vertices[1] - model[i].triangle.vertices[0];
		glm::vec3 e1 = model[i].triangle.vertices[2] - model[i].triangle.vertices[0];
		glm::vec3 SPVector = cameraPosition - model[i].triangle.vertices[0];
		glm::mat3 DEMatrix(-rayDirection, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

		float t = possibleSolution[0];
		float u = possibleSolution[1];
		float v = possibleSolution[2];
		if ((t>0.0f) && (u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && ((u + v) <= 1.0) && (t<intersection.distanceFromCamera) && (i != ignoreIndex)) {
			intersection.intersectionPoint = model[i].triangle.vertices[0] + u * e0 + v * e1;
			intersection.distanceFromCamera = t;
			intersection.intersectedTriangle = model[i].triangle;
			intersection.triangleIndex = i;
		}
	}
	return intersection;
}

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
			RayTriangleIntersection closest = getClosestIntersection(model, ray.intersectionPoint + glm::normalize(relativePos)*0.01f, glm::normalize(relativePos), ray.triangleIndex);
			bool occluded = (closest.distanceFromCamera < distance) && (closest.distanceFromCamera != std::numeric_limits<float>::infinity());
			if (!occluded) w++;
			n++;
		} 
	}
	return w/n;
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

glm::vec3 getphongNormal(RayTriangleIntersection intersection, std::vector<Face> model) {
    glm::vec3 barycentric = getBarycentricOfIntersection(intersection);
    glm::vec3 normal = barycentric.z * model[intersection.triangleIndex].v0Normal 
        + barycentric.x * model[intersection.triangleIndex].v1Normal 
        + barycentric.y * model[intersection.triangleIndex].v2Normal;

    return glm::normalize(normal);
}

glm::vec3 getNormal(RayTriangleIntersection intersection, std::vector<Face> model) {
	if (model[intersection.triangleIndex].phong) return getphongNormal(intersection, model);
	else return intersection.intersectedTriangle.normal;
}

Colour getEnvironment(glm::vec3 ray, cameraClass &camera) {
	// Not my code 
	float u = atan2(ray.z, ray.x); 	
    if (u < 0) u += 2.0f * M_PI;
    u /= 2.0f * M_PI;
	float v = acos(glm::clamp(ray.y, -1.0f, 1.0f)) / M_PI;
    int x = glm::clamp(int(u * camera.environment.width), 0, int(camera.environment.width) - 1);
    int y = glm::clamp(int(v * camera.environment.height), 0, int(camera.environment.height) - 1);
	// end of not my code 
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

glm::vec3 randomDiffuse(glm::vec3 normal) {
	float z = (float(rand())/(float)RAND_MAX); 
	float t = 2.0f * M_PI * (float(rand())/(float)RAND_MAX);
	float r = sqrt(1 - z*z);
	glm::vec3 direction = glm::normalize(glm::vec3(r * cos(t), r * sin(t), z));

	glm::vec3 u = glm::normalize(glm::cross(glm::vec3(0,1,0), normal));
	glm::vec3 v = glm::cross(normal, u);
	return glm::normalize(u*direction.x + v*direction.y + normal*direction.z);
}

Colour getColour(std::vector<Face> &model, node* kdTree, RayTriangleIntersection intersection, glm::vec3 ray, glm::vec3 normal, Face face, cameraClass &camera, int recursionDepth) {
	if (intersection.distanceFromCamera == std::numeric_limits<float>::infinity()) {
		return getEnvironment(ray, camera);
    } else if (face.mirror && (recursionDepth > 0)) {
		glm::vec3 R = glm::reflect(ray, normal);;
		RayTriangleIntersection reflected = getClosestIntersection(model, intersection.intersectionPoint + normal*0.05f, R, intersection.triangleIndex);
		if (reflected.distanceFromCamera != std::numeric_limits<float>::infinity()) {
			glm::vec3 reflectedNormal = getNormal(reflected, model);
        	return getColour(model, kdTree, reflected, R, reflectedNormal, model[reflected.triangleIndex], camera, recursionDepth - 1);
		} else {
			return getEnvironment(R, camera);
		}
	} else if ((face.opacity<1.0f) && (recursionDepth > 0)) {
		glm::vec3 refracted;
		//glm::vec3 N = glm::normalize(face.triangle.normal);
		glm::vec3 N = normal;
		float ratio;
		if (glm::dot(ray, N) < 0.0f) { //Ray entering
			ratio = (1.0f/1.5f);
			refracted = glm::refract(ray, N, ratio); 
		} else { //Ray exiting 
			N = -N;
			ratio = (1.5f/1.0f);
			refracted = glm::refract(ray, N, ratio); 
		}
		if (glm::length(refracted) < 1e-6f) {  // TIR
			glm::vec3 r = glm::normalize(glm::reflect(ray, N));
			RayTriangleIntersection newIntersection = getClosestIntersection(model, intersection.intersectionPoint + r*0.05f, r, intersection.triangleIndex);
			if (newIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
				glm::vec3 newIntersectionNormal = getNormal(newIntersection, model);
				return getColour(model, kdTree, newIntersection, r, newIntersectionNormal, model[newIntersection.triangleIndex], camera, recursionDepth-1);
			} else {
				return getEnvironment(r, camera);
			}
		} else { // Refraction
			refracted = glm::normalize(refracted);
			RayTriangleIntersection newIntersection = getClosestIntersection(model, intersection.intersectionPoint + refracted*0.001f, refracted, intersection.triangleIndex);
			if (newIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {	
         		glm::vec3 newIntersectionNormal = getNormal(newIntersection, model);
            	return getColour(model, kdTree, newIntersection, refracted, newIntersectionNormal, model[newIntersection.triangleIndex], camera, recursionDepth-1);
        	} else {
            	return getEnvironment(refracted, camera);
        	}
		}
	} else {
		glm::vec3 colour;
		if (face.textured) {
			glm::vec3 barycentric = getBarycentricOfIntersection(intersection);
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

		float ambient = 0.2f; 
		float distance = glm::length(camera.light - intersection.intersectionPoint);
		float proximity = 1.0f / (distance * distance + 1.0f); 
		float angle = std::fmax(glm::dot(glm::normalize(camera.light - intersection.intersectionPoint), normal), 0.0f);
		float specular = specularValue(intersection, normal, face.specularExponent, camera);
		float shadow = (0.2f + inShadow(model, intersection, camera.light, 2));
		
		
		glm::vec3 photonTerm = searchkdTreeForN(kdTree, intersection, normal, 500) * 2.0f + searchkdTreeForN(kdTree, intersection, normal, 50);
		glm::vec3 ambientTerm = face.ambient * ambient;
		glm::vec3 diffuseTerm = colour * angle * proximity * shadow;
		glm::vec3 specularTerm = face.specular * specular * shadow;
		glm::vec3 total = glm::min((ambientTerm + diffuseTerm + specularTerm + photonTerm), glm::vec3(1.0f));
		Colour output = Colour(total[0]*255.0f, total[1]*255.0f, total[2]*255.0f);

		return (output);
	}
}

void traceRay(std::vector<Face> &model, std::vector<photon> &output, glm::vec3 origin, glm::vec3 direction, glm::vec3 power, int recursionDepth) {
	RayTriangleIntersection intersection = getClosestIntersection(model, origin, direction, -1);
	Colour colour;
	photon p;
	if (recursionDepth <= 0) return;
	else if ((intersection.triangleIndex >= 0) && (intersection.distanceFromCamera != std::numeric_limits<float>::infinity())) {
		glm::vec3 location = glm::vec3(intersection.intersectionPoint);
		glm::vec3 normal = getNormal(intersection, model);
		if (model[intersection.triangleIndex].opacity < 1.0f) { //Material transparent
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
				traceRay(model, output, intersection.intersectionPoint + R*0.05f, R, power, recursionDepth-1);
			} else { //Refraction
				refracted = glm::normalize(refracted);
				glm::vec3 newPower = power * model[intersection.triangleIndex].transmission;
				traceRay(model, output, intersection.intersectionPoint + refracted*0.001f, refracted, newPower, recursionDepth-1);
			}
		} else {
			if (model[intersection.triangleIndex].mirror) { //Mirror, reflect all
				glm::vec3 R = glm::normalize(glm::reflect(direction, normal));
				traceRay(model, output, intersection.intersectionPoint + normal*0.05f, R, power, recursionDepth-1);
			} else { //Other surface, russian roulette for reflection
				glm::vec3 newPower = power * model[intersection.triangleIndex].diffuse;
				p.location = location;
				p.power = newPower;
				p.incidence = direction;
				output.push_back(p);
				float prob = fmax(model[intersection.triangleIndex].diffuse[0], fmax(model[intersection.triangleIndex].diffuse[1], model[intersection.triangleIndex].diffuse[2]));
				float roulette = ((float)rand()/(float)RAND_MAX < prob); // reflects with 50% probability
				if (roulette) {
					glm::vec3 direction = randomDiffuse(normal);
					traceRay(model, output, intersection.intersectionPoint + normal*0.05f, direction, newPower/prob, recursionDepth-1);
				}
			}
		}
	}
}

std::vector<photon> photonMap(DrawingWindow &window, std::vector<Face> &model, int count, cameraClass &camera) {
	std::cout<<"building photon map"<<std::endl;
	std::vector<photon> map;
	photon point;
	for (int i=0; i<count; i++) {
		float z = 2.0f * (float(rand())/(float)RAND_MAX) - 1.0f;
		float t = 2.0f * M_PI * (float(rand())/(float)RAND_MAX);
		float r = sqrt(1 - z*z);
		glm::vec3 direction = glm::vec3(r * cos(t), r * sin(t), z);
		float power = 1.0f;
		traceRay(model, map, camera.light, direction, glm::vec3(power/count), 6);
		if (i%1000 == 0) std::cout<<(float)i/(float)count<<std::endl;
	}
	return map;
}

void drawRaytraced(DrawingWindow &window, std::vector<Face> &model, node* kdTree, cameraClass &camera) {
	/*
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}
		*/
	
	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		lookAt(glm::vec3(0,0,0), camera);
	}

	float viewportHeight = 1.5;
	float viewportWidth = viewportHeight * (double(WIDTH)/HEIGHT);

	glm::vec3 viewportXIncrement = glm::vec3(viewportWidth, 0, 0) / (WIDTH * 1.0f);
	glm::vec3 viewportYIncrement = glm::vec3(0, -viewportHeight, 0) / (HEIGHT * 1.0f);
	glm::vec3 viewportXStart = glm::vec3(-(viewportWidth/2.0f), 0, 0);
	glm::vec3 viewportYStart = glm::vec3(0, (viewportHeight/2.0f), 0);

	SDL_Event event;
	for (float i=0; i<HEIGHT; i++) {
		if (window.pollForInputEvents(event)) handleEvent(event, window, camera);
		std::cout<<i/HEIGHT<<std::endl;
		for (float j=0; j<WIDTH; j++) {	
			glm::vec3 pixelCameraSpace = -glm::vec3(0, 0, camera.focalLength) + viewportXStart + j * viewportXIncrement + viewportYStart + i * viewportYIncrement;
			glm::vec3 rayDirection = glm::normalize(camera.cameraOri * pixelCameraSpace);
			RayTriangleIntersection intersection = getClosestIntersection(model, camera.cameraPos, rayDirection, -1);
			Colour colour;
			if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
 				glm::vec3 normal = glm::normalize(getNormal(intersection, model));
				colour = getColour(model, kdTree, intersection, rayDirection, normal, model[intersection.triangleIndex], camera, 6);
			} else {
				colour = getEnvironment(rayDirection, camera);
			}
			uint32_t pixelColour = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
			window.setPixelColour(j, i, pixelColour);
			window.renderFrame();
		}
	}
}