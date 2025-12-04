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

// make 

// PLEASE NOTE:
//- gouraud shading is implemented but not enabled, nor is it used in the ident, it has been fully replaced by phong shading 
//- the ident only includes DOF for the first second. This is due to the immense increase in render time it produces

// wireframe 
void drawWireFrame(DrawingWindow &window, std::vector<Face>& model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
	// reset canvas to black
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}

	// initialise z buffer to 0 
	std::fill(&zbuf[0][0], &zbuf[0][0] + HEIGHT * WIDTH, 0.0f);

	// if orbiting, increment orbit
	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		camera.lookAt(glm::vec3(0,0,0));
	}

	// print light as white dot 
	glm::vec3 p = glm::transpose(camera.cameraOri) * (camera.light - camera.cameraPos); // apply camera pos / orientation 
	CanvasPoint light = camera.projectVertexOntoCanvasPoint(camera.light); // project into canvas coordinates
	if (light.x < WIDTH && light.x > 0 && light.y < HEIGHT && light.y > 0 && p.z < 0.0f) {  
		window.setPixelColour(light.x, light.y, 0xFFFFFFFF);
	}

		
	// loop over all triangles
	for (int i=0; i<model.size(); i++) {
		// apply camera pos/orientation to all 3 verticies 
		glm::vec3 p0 = model[i].triangle.vertices[0];
		glm::vec3 p1 = model[i].triangle.vertices[1];
		glm::vec3 p2 = model[i].triangle.vertices[2];
		glm::vec3 cam0 = glm::transpose(camera.cameraOri) * (p0 - camera.cameraPos);
   		glm::vec3 cam1 = glm::transpose(camera.cameraOri) * (p1 - camera.cameraPos);
    	glm::vec3 cam2 = glm::transpose(camera.cameraOri) * (p2 - camera.cameraPos);
		// if any are behind the camera, the projection does not work. Ignore the whole polygon. 
		if (cam0.z > 0.0f || cam1.z > 0.0f || cam2.z > 0.0f) {
				continue;
		} else {
			// draw 3 lines connecting all verticies
			CanvasPoint v0 = camera.projectVertexOntoCanvasPoint(p0);
			CanvasPoint v1 = camera.projectVertexOntoCanvasPoint(p1);
			CanvasPoint v2 = camera.projectVertexOntoCanvasPoint(p2);
			drawLine(window, zbuf, v1, v2, model[i].getColour());
			drawLine(window, zbuf, v0, v2, model[i].getColour());
			drawLine(window, zbuf, v0, v1, model[i].getColour());
		}
	}
		
}

// rasterised 
void drawRasterised(DrawingWindow &window, std::vector<Face>& model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
    // reset canvas to black
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}

	// initialise z buffer to 0 
	std::fill(&zbuf[0][0], &zbuf[0][0] + HEIGHT * WIDTH, 0.0f);

	// if orbiting, increment orbit
	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		camera.lookAt(glm::vec3(0,0,0));
	}
		
	// loop over all triangles
	for (int i=0; i<model.size(); i++) {	
		glm::vec3 p0 = model[i].triangle.vertices[0];
		glm::vec3 p1 = model[i].triangle.vertices[1];
		glm::vec3 p2 = model[i].triangle.vertices[2];
		glm::vec3 cam0 = glm::transpose(camera.cameraOri) * (p0 - camera.cameraPos);
   		glm::vec3 cam1 = glm::transpose(camera.cameraOri) * (p1 - camera.cameraPos);
    	glm::vec3 cam2 = glm::transpose(camera.cameraOri) * (p2 - camera.cameraPos);
		// if any are behind the camera, the projection does not work. Ignore the whole polygon. 
		if (cam0.z > 0.0f || cam1.z > 0.0f || cam2.z > 0.0f) {
				continue;
		} else {
			if (model[i].textured) {
				// draw textured triangle 
				triangleTextured3D(window, zbuf, model[i], camera);
			} else {
				// draw untextured triangle
				triangle3D(window, zbuf, model[i].triangle, camera);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	srand(time(0));
	float zbuf[HEIGHT][WIDTH];

	// load scene
	std::vector<Face> model = loadTriangle("textured-cornell-box.obj", 0.35);
    
	// initialise camera class settings
	cameraClass camera;
	camera.light = glm::vec3(0, 1*0.35, 0); // light location
	camera.environment = TextureMap("skybox.ppm");
	camera.cameraPos = glm::vec3(0.0, 0.0, 4.0);
	camera.cameraOri = glm::mat3(1.0f);
	camera.orbit = false;
	camera.focalLength = 2.0;
	camera.focalDistance = 4.0;
	camera.mode = "rasterised";
	camera.lensRadius = 0.01f; // increases DOF
	camera.dofSamples = 1; // increases quality of DOF
	camera.viewportHeight = 1.5;
	camera.viewportWidth = camera.viewportHeight * ((float)WIDTH/(float)HEIGHT);
	camera.exposure = 1.0f; // increases brightness
	camera.shadowSamples = 1; // increases shadow quality 

	std::vector<photon> lightMap;
	std::ifstream f("photonMap.txt");
    if (f.good()) {
		// if there is an existing photon map, load it 
		std::cout<<"Loading photon map"<<std::endl;
		lightMap = getPhotonMap("photonMap.txt");
	} else {
		// if there is no existing photon map, make one 
		std::cout<<"Building photon map"<<std::endl;
		lightMap = photonMap(model, 100000, camera); // adjust photon number here (at least 1 million for good results, but expensive)
		storePhotonMap(lightMap, "photonMap.txt");
	}
	// with the photon map as a vector, build a kd tree
	node* kdTree = buildkdTree(lightMap, 0, lightMap.size()-1, 0);

	// produce vertex brightnesses for gouraud shading
    getVertexColours(model, kdTree, camera);

	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	int frame = 0;
	while (true) {
		float start_time = SDL_GetTicks();
		// poll for events
		if (window.pollForInputEvents(event)) camera.handleEvent(event, window);
		// focus camera to change focal distance to distance of object currently looking at 
		focusCamera(model, camera);
		if (camera.mode == "raytraced") {
			// delagate to raytraced drawer
			// adjust thread number to cpu thread count, leave at one for no concurrency
			int threadNumber = 1;
			drawRaytraced(window, model, kdTree, camera, threadNumber);
		} else if (camera.mode == "rasterised") {
			// delagate to rasterised drawer
			drawRasterised(window, model, zbuf, camera);
		} else if (camera.mode == "wireframe") {
			// delagate to wireframe drawer
			drawWireFrame(window, model, zbuf, camera);
		}
		// render the frame
		window.renderFrame();
		// print frame time and count 
		std::cout<<"Frame: " + std::to_string(frame) + " Time: " + std::to_string((SDL_GetTicks() - start_time)/1000) + "s" <<std::endl;

		// track code, produces movement through demo scene
		// only works for demo scene used in 15 second render. Disabled for cornell box. 
		/* 
		if (frame < 25) {
		    camera.exposure = 20.0f;
			camera.cameraOri = yMatrix(0.01, 1) * camera.cameraOri;
		} else if (frame < 75) {
			camera.cameraPos.x -= 0.04f;
		} else if (frame < 100) {
			camera.cameraPos.z -= 0.04f;
		} else if (frame < 150) {
			camera.cameraPos.z -= 0.04f;
			camera.cameraPos.x += 0.04f;
		} else if (frame < 200) {
			camera.cameraPos.z += 0.04f;
			camera.cameraPos.x += 0.04f;
		} else if (frame < 250) {
			camera.cameraPos.x -= 0.04f;
			camera.cameraOri = yMatrix(0.005, -1) * camera.cameraOri;
			camera.focalLength -= 0.01;
		} else if (frame < 350) {
			if (frame < 260) {
				camera.exposure -= 1.9f;
			}
			camera.cameraOri = xMatrix(0.0314, -1) * camera.cameraOri;
			camera.cameraPos.z -= 0.01f;
		}
		*/
		window.saveBMP("output/output" + std::to_string(frame) + ".bmp");
		frame++;
		
	}
}
