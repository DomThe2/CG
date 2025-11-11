#include <algorithm>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>

#include "CanvasPoint.h"
#include "camera.h"
#include "Utils.h"

glm::mat3 xMatrix(float angle, float direction) {
	return glm::mat3(
    cos(angle), 0, direction*sin(angle),
	0, 1, 0,
    -direction*sin(angle), 0, cos(angle));
}
glm::mat3 yMatrix(float angle, float direction) {
	return glm::mat3(
	1, 0, 0,
    0, cos(angle), -direction*sin(angle),
    0, direction*sin(angle), cos(angle));

}
void lookAt(glm::vec3 location, cameraClass &camera) {
	glm::vec3 forward = glm::normalize(camera.cameraPos - location);
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), forward));
	glm::vec3 up = glm::cross(forward, right);
	camera.cameraOri = glm::mat3(right, up, forward);
	
}

std::vector<float> interpolateSingleFloats(float from, float to, float numberOfValues) {
	std::vector<float> output;
	if (numberOfValues <= 1) {
		output.push_back(from);
	} else {
		for (int i=0; i<numberOfValues; i++) {
			output.push_back(from + (((to - from)/(numberOfValues-1.0)) * i));
		}
	}
	return output;
}

CanvasPoint projectVertexOntoCanvasPoint(cameraClass &camera, glm::vec3 vertexPosition) {
	glm::vec3 relativePosition = glm::transpose(camera.cameraOri) * (vertexPosition - camera.cameraPos);
    float x = std::round((camera.focalLength * (relativePosition.x / relativePosition.z)) * (WIDTH/2.0f) + (WIDTH / 2.0f));	
    float y = std::round((camera.focalLength * (relativePosition.y / relativePosition.z)) * (WIDTH/2.0f) + (HEIGHT / 2.0f));
	float depth = -1.0/relativePosition.z;
	x = WIDTH - x;
	return CanvasPoint(x, y, depth);
} 

std::vector<std::string> split(const std::string &line, char delimiter) {
	auto haystack = line;
	std::vector<std::string> tokens;
	size_t pos;
	while ((pos = haystack.find(delimiter)) != std::string::npos) {
		tokens.push_back(haystack.substr(0, pos));
		haystack.erase(0, pos + 1);
	}
	// Push the remaining chars onto the vector
	tokens.push_back(haystack);
	return tokens;
}

// Uses Cramer’s rule to convert from 2D coordinates to Barycentric proportional proximities
glm::vec3 convertToBarycentricCoordinates(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, glm::vec2 r)
{
    glm::vec2 e0 = v1 - v0;
    glm::vec2 e1 = v2 - v0;
    glm::vec2 e2 = r - v0;
    float d00 = glm::dot(e0, e0);
    float d01 = glm::dot(e0, e1);
    float d11 = glm::dot(e1, e1);
    float d20 = glm::dot(e2, e0);
    float d21 = glm::dot(e2, e1);
    float denominator = d00 * d11 - d01 * d01;
    float u = (d11 * d20 - d01 * d21) / denominator;
    float v = (d00 * d21 - d01 * d20) / denominator;
    float w = 1.0f - u - v;
    return glm::vec3(u,v,w);
}

glm::vec3 convertToBarycentricCoordinates(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 r)
{
    glm::vec3 e0 = v1 - v0;
    glm::vec3 e1 = v2 - v0;
    glm::vec3 e2 = r - v0;
    float d00 = glm::dot(e0, e0);
    float d01 = glm::dot(e0, e1);
    float d11 = glm::dot(e1, e1);
    float d20 = glm::dot(e2, e0);
    float d21 = glm::dot(e2, e1);
    float denominator = d00 * d11 - d01 * d01;
    float u = (d11 * d20 - d01 * d21) / denominator;
    float v = (d00 * d21 - d01 * d20) / denominator;
    float w = 1.0f - u - v;
    return glm::vec3(u,v,w);
}
