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
#include <parser.h>
#include <kdTree.h>

// cd '01 Introduction and Orientation'/extras/RedNoise
// make 
// make speedy

void drawWireFrame(DrawingWindow &window, std::vector<Face>& model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}

	glm::vec3 p = glm::transpose(camera.cameraOri) * (camera.light - camera.cameraPos);
	CanvasPoint light = camera.projectVertexOntoCanvasPoint(camera.light);
	if (light.x < WIDTH && light.x > 0 && light.y < HEIGHT && light.y > 0 && p.z < 0.0f) {
		window.setPixelColour(light.x, light.y, 0xFFFFFFFF);
	}

	std::fill(&zbuf[0][0], &zbuf[0][0] + HEIGHT * WIDTH, 0.0f);

	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		camera.lookAt(glm::vec3(0,0,0));
	}
		
	for (int i=0; i<model.size(); i++) {
		glm::vec3 p0 = model[i].triangle.vertices[0];
		glm::vec3 p1 = model[i].triangle.vertices[1];
		glm::vec3 p2 = model[i].triangle.vertices[2];
		glm::vec3 cam0 = glm::transpose(camera.cameraOri) * (p0 - camera.cameraPos);
   		glm::vec3 cam1 = glm::transpose(camera.cameraOri) * (p1 - camera.cameraPos);
    	glm::vec3 cam2 = glm::transpose(camera.cameraOri) * (p2 - camera.cameraPos);
		if (cam0.z > 0.0f || cam1.z > 0.0f || cam2.z > 0.0f) {
				continue;
		} else {
			CanvasPoint v0 = camera.projectVertexOntoCanvasPoint(p0);
			CanvasPoint v1 = camera.projectVertexOntoCanvasPoint(p1);
			CanvasPoint v2 = camera.projectVertexOntoCanvasPoint(p2);
			drawLine(window, zbuf, v1, v2, model[i].getColour());
			drawLine(window, zbuf, v0, v2, model[i].getColour());
			drawLine(window, zbuf, v0, v1, model[i].getColour());
		}
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
		camera.lookAt(glm::vec3(0,0,0));
	}
		
	for (int i=0; i<model.size(); i++) {	
		glm::vec3 p0 = model[i].triangle.vertices[0];
		glm::vec3 p1 = model[i].triangle.vertices[1];
		glm::vec3 p2 = model[i].triangle.vertices[2];
		glm::vec3 cam0 = glm::transpose(camera.cameraOri) * (p0 - camera.cameraPos);
   		glm::vec3 cam1 = glm::transpose(camera.cameraOri) * (p1 - camera.cameraPos);
    	glm::vec3 cam2 = glm::transpose(camera.cameraOri) * (p2 - camera.cameraPos);
		if (cam0.z > 0.0f || cam1.z > 0.0f || cam2.z > 0.0f) {
				continue;
		} else {
			if (model[i].textured) {
				triangleTextured3D(window, zbuf, model[i], camera);
			} else {
				triangle3D(window, zbuf, model[i].triangle, camera);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	srand(time(0));
	float zbuf[HEIGHT][WIDTH];
	//camera.light = glm::vec3(0, 1*0.35, 0);
	std::vector<Face> model = loadTriangle("demoScene.obj", 0.35);

	cameraClass camera;
	camera.light = glm::vec3(0.0f, 5.0f, 14.6f) * 0.35f;
	camera.environment = TextureMap("skybox.ppm");
	camera.cameraPos = glm::vec3(0.0, 1.0, 4.0);
	camera.cameraOri = glm::mat3(1.0f);
	camera.orbit = false;
	camera.focalLength = 2.0;
	camera.focalDistance = 4.0;
	camera.mode = "wireframe";
	camera.lensRadius = 0.01f; 
	camera.dofSamples = 1;
	camera.viewportHeight = 1.5;
	camera.viewportWidth = camera.viewportHeight * ((float)WIDTH/(float)HEIGHT);
	camera.exposure = 20.0f;
	camera.shadowSamples = 1;
	

	std::vector<photon> lightMap;
	std::ifstream f("photonMap.txt");
    if (f.good()) {
		std::cout<<"Loading photon map"<<std::endl;
		lightMap = getPhotonMap("photonMap.txt");
	} else {
		std::cout<<"Building photon map"<<std::endl;
		lightMap = photonMap(model, 1000000, camera);
		storePhotonMap(lightMap, "photonMap.txt");
	}
	node* kdTree = buildkdTree(lightMap, 0, lightMap.size()-1, 0);

	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	int frame = 0;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		float start_time = SDL_GetTicks();
		if (window.pollForInputEvents(event)) camera.handleEvent(event, window);
		if (camera.mode == "raytraced") {
			drawRaytraced(window, model, kdTree, camera, 4);
		} else if (camera.mode == "rasterised") {
			drawRasterised(window, model, zbuf, camera);
		} else if (camera.mode == "wireframe") {
			drawWireFrame(window, model, zbuf, camera);
		}
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
		std::cout<<"Frame: " + std::to_string(frame) + " Time: " + std::to_string((SDL_GetTicks() - start_time)/1000) + "s" <<std::endl;
		//camera.focalDistance = 4.2f*0.35f;
		if (frame < 25) {
			camera.cameraOri = yMatrix(0.01, 1) * camera.cameraOri;
		} else if (frame < 75) {
			camera.cameraPos.x -= 0.04f;
		} else if (frame < 100) {
			//if (frame < 85) camera.focalDistance += 0.33 * 0.35;
			camera.cameraPos.z -= 0.04f;
			//camera.focalDistance = 7.5 * 0.35;
		} else if (frame < 150) {
			//if (frame < 110) camera.focalDistance -= 0.15 * 0.35;
			camera.cameraPos.z -= 0.04f;
			camera.cameraPos.x += 0.04f;
			//camera.focalDistance = 6 * 0.35;
			//if (frame > 140) camera.exposure -= 0.7f;
		} else if (frame < 200) {
			if (frame < 160) {
				//camera.focalDistance += 0.15 * 0.35;
				//camera.exposure += 0.7f;
			}
			camera.cameraPos.z += 0.04f;
			camera.cameraPos.x += 0.04f;
			//camera.focalDistance = 7.5 * 0.35;
		} else if (frame < 250) {
			camera.cameraPos.x -= 0.04f;
			camera.cameraOri = yMatrix(0.005, -1) * camera.cameraOri;
			camera.focalLength -= 0.01;
		} else if (frame < 350) {
			if (frame < 260) {
				//camera.focalDistance += 0.1 * 0.35;
				camera.exposure -= 1.9f;
			}
			camera.cameraOri = xMatrix(0.0314, -1) * camera.cameraOri;
			camera.cameraPos.z -= 0.01f;
			//camera.focalDistance = 8.5 * 0.35;
		}
		focusCamera(model, camera);
		if (frame <= 375) window.saveBMP("output/output" + std::to_string(frame) + ".bmp");
		frame++;
		
	}
}
