#include <vector>
#include <glm/glm.hpp>

#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "CanvasTriangle.h"
#include "ModelTriangle.h"
#include "Colour.h"
#include "Utils.h"
#include "camera.h"

#include "filledTriangle.h"

void drawLine(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasPoint from, CanvasPoint to, Colour colour) {
	float number = fmax(std::fabs(to.x - from.x), std::fabs(to.y - from.y))+1.0;
	number = std::ceil(number);
	std::vector<float> x = interpolateSingleFloats(from.x, to.x, number);
	std::vector<float> y = interpolateSingleFloats(from.y, to.y, number);
	std::vector<float> depth = interpolateSingleFloats(from.depth, to.depth, number);
	
	uint32_t pixelColour = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
	
	for (int i=0; i<number; i++) {
		int xR = (int)std::round(x[i]);
		int yR = (int)std::round(y[i]);
		if (xR >= WIDTH || yR >= HEIGHT || xR < 0 || yR < 0) {
			continue; //Check if out of bounds of buffer
		} else if (zbuf[yR][xR] <= depth[i]) { 
			//If nothing is in front of pixel, update window and zbuffer 
			window.setPixelColour(xR, yR, pixelColour);
			zbuf[yR][xR] = depth[i];
		}
	}
}

void drawFlatBottomTriangle(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasPoint v0, CanvasPoint v1, CanvasPoint v2, Colour fill) {
	float height = v0.y - v1.y;
	float width = fmax(std::fabs(v0.x-v1.x), std::fabs(v1.x-v2.x));
	int numSteps = (int)std::round(fmax(std::fabs(height), width)) + 1;
	std::vector<float> leftX = interpolateSingleFloats(v0.x, v1.x, numSteps);
	std::vector<float> leftD = interpolateSingleFloats(v0.depth, v1.depth, numSteps);
	std::vector<float> rightX = interpolateSingleFloats(v0.x, v2.x, numSteps);
	std::vector<float> rightD = interpolateSingleFloats(v0.depth, v2.depth, numSteps);
	std::vector<float> Y = interpolateSingleFloats(v0.y, v1.y, numSteps);
	
	for (int i = 0; i<numSteps; i++) {
		drawLine(window, zbuf, CanvasPoint(leftX[i], Y[i], leftD[i]), CanvasPoint(rightX[i], Y[i], rightD[i]), fill);	
	}
}

void filledTriangle(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasTriangle points, Colour fill) {
	CanvasPoint v0 = points.v0();
	CanvasPoint v1 = points.v1();
	CanvasPoint v2 = points.v2();

	if (v0.y > v1.y) std::swap(v0, v1);
	if (v1.y > v2.y) std::swap(v1, v2);
	if (v0.y > v1.y) std::swap(v0, v1);

	if (v2.y - v0.y < 1.0f) {
		drawLine(window, zbuf, v0, v2, fill);
	 	return;
	}
	
    float x = (v0.x + ((v1.y - v0.y) / (v2.y - v0.y)) * (v2.x - v0.x));
	float d = (v0.depth + ((v1.y - v0.y) / (v2.y - v0.y)) * (v2.depth - v0.depth));
	CanvasPoint midpoint = CanvasPoint(x, v1.y, d);
	
	if (std::fabs(v0.y - v1.y) < 1.0f) { //Should be redundant?
		drawFlatBottomTriangle(window, zbuf, v2, v1, v0, fill);
	} else if (std::fabs(v1.y - v2.y) < 1.0f) {
		drawFlatBottomTriangle(window, zbuf, v0, v1, v2, fill);
	} else {
	drawFlatBottomTriangle(window, zbuf, v0, v1, midpoint, fill);
	drawFlatBottomTriangle(window, zbuf, v2, v1, midpoint, fill);
	}
}

void triangle3D(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], ModelTriangle triangle, cameraClass &camera) {
	CanvasPoint v0 = projectVertexOntoCanvasPoint(camera, triangle.vertices[0]);
	CanvasPoint v1 = projectVertexOntoCanvasPoint(camera, triangle.vertices[1]);
	CanvasPoint v2 = projectVertexOntoCanvasPoint(camera, triangle.vertices[2]);
	
	glm::vec3 center = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) / 3.0f;
	float epsilon = center.z * 0.00001f;
	v0.depth += epsilon;
	v1.depth += epsilon;
	v2.depth += epsilon;

	CanvasTriangle flatTriangle = CanvasTriangle(v0, v1, v2);

	filledTriangle(window, zbuf, flatTriangle, triangle.colour);
}