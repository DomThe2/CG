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