#include <vector>
#include <glm/glm.hpp>

#include "DrawingWindow.h"
#include "CanvasPoint.h"
#include "CanvasTriangle.h"
#include "ModelTriangle.h"
#include "Colour.h"
#include "Utils.h"
#include "camera.h"
#include "face.h"
#include "TextureMap.h"

#include "texturedTriangle.h"

void drawHorizontalTexturedLine(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], TextureMap texture, CanvasPoint from, CanvasPoint to) {
	float number = abs(to.x - from.x) + 1;
	std::vector<float> x = interpolateSingleFloats(from.x, to.x, number);
	std::vector<float> u = interpolateSingleFloats(from.texturePoint.x, to.texturePoint.x, number);
	std::vector<float> v = interpolateSingleFloats(from.texturePoint.y, to.texturePoint.y, number);
	std::vector<float> depth = interpolateSingleFloats(from.depth, to.depth, number);
	for (int i=0; i<number; i++) {
		int xR = (int)std::round(x[i]);
		int yR = (int)std::round(from.y);
		if (xR >= WIDTH || yR >= HEIGHT || xR < 0 || yR < 0) {
			continue; //Check if out of bounds of buffer
		} else if (zbuf[yR][xR] <= depth[i]) { 
			window.setPixelColour(xR, yR, texture.pixels[(int)v[i] * texture.width + (int)u[i]]);
			zbuf[yR][xR] = depth[i];
		}
	}
}

void drawFlatBottomTexturedTriangle(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasPoint v0, CanvasPoint v1, CanvasPoint v2, TextureMap texture) {
	float height = v0.y - v1.y;
	float width = fmax(std::fabs(v0.x-v1.x), std::fabs(v1.x-v2.x));
	int numSteps = (int)std::round(fmax(std::fabs(height), width)) + 1;
	std::vector<float> leftX = interpolateSingleFloats(v0.x, v1.x, numSteps);
	std::vector<float> leftD = interpolateSingleFloats(v0.depth, v1.depth, numSteps);
	std::vector<float> rightX = interpolateSingleFloats(v0.x, v2.x, numSteps);
	std::vector<float> rightD = interpolateSingleFloats(v0.depth, v2.depth, numSteps);
	std::vector<float> leftU = interpolateSingleFloats(v0.texturePoint.x, v1.texturePoint.x, numSteps);
	std::vector<float> leftV = interpolateSingleFloats(v0.texturePoint.y, v1.texturePoint.y, numSteps);
	std::vector<float> rightU = interpolateSingleFloats(v0.texturePoint.x, v2.texturePoint.x, numSteps);
	std::vector<float> rightV = interpolateSingleFloats(v0.texturePoint.y, v2.texturePoint.y, numSteps);
	std::vector<float> Y = interpolateSingleFloats(v0.y, v1.y, numSteps);
	
	for (int i = 0; i<numSteps; i++) {
		CanvasPoint from = CanvasPoint(leftX[i], Y[i], leftD[i]);
		CanvasPoint to = CanvasPoint(rightX[i], Y[i], rightD[i]);
		from.texturePoint = TexturePoint(leftU[i], leftV[i]);
		to.texturePoint = TexturePoint(rightU[i], rightV[i]);
		drawHorizontalTexturedLine(window, zbuf, texture, from, to);	
	}
}

void texturedTriangle(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasTriangle points, TextureMap texture, std::array<TexturePoint, 3> texturePoints) {
	CanvasPoint v0 = points.v0();
	CanvasPoint v1 = points.v1();
	CanvasPoint v2 = points.v2();

	v0.texturePoint = texturePoints[0];
	v1.texturePoint = texturePoints[1];
	v2.texturePoint = texturePoints[2];

	if (v0.y > v1.y) std::swap(v0, v1);
	if (v1.y > v2.y) std::swap(v1, v2);
	if (v0.y > v1.y) std::swap(v0, v1);

	/*
	if (v0.y == v2.y) {
		//drawLine(window, v0, v2, Colour(255,255,255));
		return;
	}
	*/

	float x = (v0.x + ((v1.y - v0.y) / (v2.y - v0.y)) * (v2.x - v0.x));
	float u = (v0.texturePoint.x + ((v1.y - v0.y) / (v2.y - v0.y)) * (v2.texturePoint.x - v0.texturePoint.x));
	float v = (v0.texturePoint.y + ((v1.y - v0.y) / (v2.y - v0.y)) * (v2.texturePoint.y - v0.texturePoint.y));
	float d = (v0.depth + ((v1.y - v0.y) / (v2.y - v0.y)) * (v2.depth - v0.depth));
	CanvasPoint midpoint = CanvasPoint(x, v1.y, d);
	midpoint.texturePoint = TexturePoint(u, v);

	drawFlatBottomTexturedTriangle(window, zbuf, v0, v1, midpoint, texture);
	drawFlatBottomTexturedTriangle(window, zbuf, v2, v1, midpoint, texture);
}

void triangleTextured3D(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], Face face, cameraClass &camera) {
	CanvasPoint v0 = projectVertexOntoCanvasPoint(camera, face.triangle.vertices[0]);
	CanvasPoint v1 = projectVertexOntoCanvasPoint(camera, face.triangle.vertices[1]);
	CanvasPoint v2 = projectVertexOntoCanvasPoint(camera, face.triangle.vertices[2]);
	
	glm::vec3 center = (face.triangle.vertices[0] + face.triangle.vertices[1] + face.triangle.vertices[2]) / 3.0f;
	float epsilon = center.z * 0.00001f;
	v0.depth += epsilon;
	v1.depth += epsilon;
	v2.depth += epsilon;
	
	CanvasTriangle flatTriangle = CanvasTriangle(v0, v1, v2);
	texturedTriangle(window, zbuf, flatTriangle, face.texture, face.triangle.texturePoints);
}