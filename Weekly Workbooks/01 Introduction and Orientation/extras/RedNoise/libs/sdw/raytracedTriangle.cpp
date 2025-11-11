#include <vector>
#include <glm/glm.hpp>

#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "CanvasTriangle.h"
#include "ModelTriangle.h"
#include "Colour.h"
#include "Utils.h"
#include "camera.h"
#include "RayTriangleIntersection.h"
#include "face.h"

#include "raytracedTriangle.h"

RayTriangleIntersection getClosestIntersection(std::vector<Face> model, glm::vec3 cameraPosition, glm::vec3 rayDirection) {
	RayTriangleIntersection intersection;
	intersection.distanceFromCamera = std::numeric_limits<float>::infinity();
	for (int i=0; i<model.size(); i++) {
		glm::vec3 e0 = model[i].triangle.vertices[1] - model[i].triangle.vertices[0];
		glm::vec3 e1 = model[i].triangle.vertices[2] - model[i].triangle.vertices[0];
		glm::vec3 SPVector = cameraPosition - model[i].triangle.vertices[0];
		glm::mat3 DEMatrix(-rayDirection, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

		float t = possibleSolution[0];
		float u = possibleSolution[1];
		float v = possibleSolution[2];
		if ((t>0.001f) && (u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && ((u + v) <= 1.0) && (t<intersection.distanceFromCamera)) {
			intersection.intersectionPoint = model[i].triangle.vertices[0] + u * e0 + v * e1;
			intersection.distanceFromCamera = t;
			intersection.intersectedTriangle = model[i].triangle;
			intersection.triangleIndex = i;
		}
	}
	return intersection;
}

bool inShadow(std::vector<Face> model, RayTriangleIntersection ray, glm::vec3 light) {
	glm::vec3 relativePos = light - ray.intersectionPoint;
	float distance = glm::length(relativePos);
	RayTriangleIntersection closest = getClosestIntersection(model, ray.intersectionPoint + glm::normalize(relativePos)*0.001f, glm::normalize(relativePos));
	return (closest.distanceFromCamera < distance) && closest.distanceFromCamera != std::numeric_limits<float>::infinity();
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

glm::vec3 getphongNormal(RayTriangleIntersection intersection, std::vector<Face> model, cameraClass &camera) {
    glm::vec3 barycentric = convertToBarycentricCoordinates(intersection.intersectedTriangle.vertices[0], intersection.intersectedTriangle.vertices[1], 
                                                            intersection.intersectedTriangle.vertices[2], intersection.intersectionPoint);

    glm::vec3 normal = barycentric.z * model[intersection.triangleIndex].v0Normal 
        + barycentric.x * model[intersection.triangleIndex].v1Normal 
        + barycentric.y * model[intersection.triangleIndex].v2Normal;

    return glm::normalize(normal);
}

void drawRaytraced(DrawingWindow &window, std::vector<Face> model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}
	std::fill(&zbuf[0][0], &zbuf[0][0] + HEIGHT * WIDTH, 0.0f);
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

	for (float i=0; i<HEIGHT; i++) {
		for (float j=0; j<WIDTH; j++) {	
			glm::vec3 pixelCameraSpace = -glm::vec3(0, 0, camera.focalLength) + viewportXStart + j * viewportXIncrement + viewportYStart + i * viewportYIncrement;
			glm::vec3 rayDirection = glm::normalize(camera.cameraOri * pixelCameraSpace);
			RayTriangleIntersection intersection = getClosestIntersection(model, camera.cameraPos, rayDirection);

			
			if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                float ambient = 0.2f;
                float brightness;
                glm::vec3 normal;
                if (camera.phong) {
                    normal = getphongNormal(intersection, model, camera);
                } else {
                    normal = intersection.intersectedTriangle.normal;
                }
                float distance = glm::length(camera.light - intersection.intersectionPoint);
                float proximity = 1.0f/9.0f * 1.0f/(distance*distance);
                float angle = std::fmax(glm::dot(glm::normalize(camera.light - intersection.intersectionPoint), glm::normalize(normal)), 0);
                float specular = specularValue(intersection, normal, 64, camera);
                brightness = ambient + (proximity * 1.0f + angle * 1.0f) * !inShadow(model, intersection, camera.light);
                //brightness = specular * 5.0f;
                brightness = glm::clamp(brightness, 0.0f, 1.0f);


				int red = glm::min(int(intersection.intersectedTriangle.colour.red*brightness + specular * 255.0f * 0.5f), 255);
				int green = glm::min(int(intersection.intersectedTriangle.colour.green*brightness + specular * 255.0f * 0.5f), 255);
				int blue = glm::min(int(intersection.intersectedTriangle.colour.blue*brightness + specular * 255.0f * 0.5f), 255);
				uint32_t pixelColour = (255 << 24) + (red << 16) + (green << 8) + blue;
				window.setPixelColour(j, i, pixelColour);
			}
		}
	}
}