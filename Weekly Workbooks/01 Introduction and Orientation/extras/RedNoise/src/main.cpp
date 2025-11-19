#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <CanvasPoint.h>
#include <Colour.h>
#include <CanvasTriangle.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>
#include <map>

#include <filledTriangle.h>
#include <texturedTriangle.h> 
#include <raytracedTriangle.h>
#include <camera.h>
#include <face.h>
#include <cameraUtils.h>
#include <parser.h>
#include <kdTree.h>

// cd '01 Introduction and Orientation'/extras/RedNoise
// make 
// make speedy

void drawWireFrame(DrawingWindow &window, std::vector<Face>& model, float (&zbuf)[HEIGHT][WIDTH], std::vector<photon> lightMap, cameraClass &camera) {
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}
	CanvasPoint light = projectVertexOntoCanvasPoint(camera, camera.light);
	window.setPixelColour(light.x, light.y, 0xFFFFFFFF);
	for (int i=0; i<lightMap.size(); i++) {
		CanvasPoint point = projectVertexOntoCanvasPoint(camera, lightMap[i].location);
		uint32_t pixelColour = (255 << 24) + (lightMap[i].power.red << 16) + (lightMap[i].power.green << 8) + lightMap[i].power.blue;
		window.setPixelColour(point.x, point.y, pixelColour);
	}
		

	std::fill(&zbuf[0][0], &zbuf[0][0] + HEIGHT * WIDTH, 0.0f);

	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		lookAt(glm::vec3(0,0,0), camera);
	}
		
	for (int i=0; i<model.size(); i++) {	
		CanvasPoint v0 = projectVertexOntoCanvasPoint(camera, model[i].triangle.vertices[0]);
		CanvasPoint v1 = projectVertexOntoCanvasPoint(camera, model[i].triangle.vertices[1]);
		CanvasPoint v2 = projectVertexOntoCanvasPoint(camera, model[i].triangle.vertices[2]);

		drawLine(window, zbuf, v0, v1, model[i].diffuse);
		drawLine(window, zbuf, v1, v2, model[i].diffuse);
		drawLine(window, zbuf, v2, v0, model[i].diffuse);
	}
}

void drawRasterised(DrawingWindow &window, std::vector<Face>& model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
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
		
	for (int i=0; i<model.size(); i++) {	
		if (model[i].textured) {
			triangleTextured3D(window, zbuf, model[i], camera);
		} else {
			triangle3D(window, zbuf, model[i].triangle, camera);
		}
	}
}

int main(int argc, char *argv[]) {
	srand(time(0));
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	
	float zbuf[HEIGHT][WIDTH];
	//std::tuple<std::map<std::string, Colour>, std::map<std::string, TextureMap>> palette = loadPalette("sphere.mtl");
	//std::vector<Face> output = loadTriangle("planet_spacship.obj", palette, 0.35);

	//std::tuple<std::map<std::string, Colour>, std::map<std::string, TextureMap>> palette = loadPalette("textured-cornell-box.mtl");
	std::vector<Face> output = loadTriangle("textured-cornell-box.obj", 0.35);



	cameraClass camera;
	camera.light = glm::vec3(0, 1.0*0.35, 0);
	//camera.light = glm::vec3(3, 3, 3);
	camera.environment = TextureMap("skybox.ppm");
	camera.cameraPos = glm::vec3(0.0, 0.0, 4.0);
	camera.cameraOri = glm::mat3(1.0f);
	camera.orbit = false;
	camera.focalLength = 2.0;
	camera.mode = "wireframe";

	std::vector<photon> lightMap;
	lightMap = photonMap(window, output, 10000, camera);
	storePhotonMap(lightMap, "photonMap.txt");
	//lightMap = getPhotonMap("photonMap.txt"); // FIX
	node* kdTree = buildkdTree(lightMap, 0, lightMap.size()-1, 0);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		float start_time = SDL_GetTicks();
		if (window.pollForInputEvents(event)) handleEvent(event, window, camera);
		if (camera.mode == "raytraced") {
			drawRaytraced(window, output, kdTree, camera);
		} else if (camera.mode == "rasterised") {
			drawRasterised(window, output, zbuf, camera);
		} else if (camera.mode == "wireframe") {
			drawWireFrame(window, output, zbuf, lightMap, camera);
		}
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
		std::cout<<"Frame Time: " + std::to_string((SDL_GetTicks() - start_time)/1000) + "s" <<std::endl;
	}
}
