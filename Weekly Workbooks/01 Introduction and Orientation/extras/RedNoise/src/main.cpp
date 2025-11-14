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

// cd '01 Introduction and Orientation'/extras/RedNoise
// make 
// make speedy

/*
std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues) {
	std::vector<glm::vec3> output;
	std::vector<float> x = interpolateSingleFloats(from.x, to.x, numberOfValues);
	std::vector<float> y = interpolateSingleFloats(from.y, to.y, numberOfValues);
	std::vector<float> z = interpolateSingleFloats(from.z, to.z, numberOfValues);
	for (int i=0; i<numberOfValues; i++) {
		output.push_back(glm::vec3(x[i], y[i], z[i]));
	}
	return output; 
}	

void strokedTriangle(DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], CanvasTriangle points, Colour colour) {
	drawLine(window, zbuf, points.v0(), points.v1(), colour);
	drawLine(window, zbuf, points.v1(), points.v2(), colour);
	drawLine(window, zbuf, points.v2(), points.v0(), colour);
}
*/

void handleEvent(SDL_Event event, DrawingWindow &window, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_o) camera.orbit = !camera.orbit;
		else if (event.key.keysym.sym == SDLK_a) camera.cameraPos.x -= 0.05f;
		else if (event.key.keysym.sym == SDLK_d) camera.cameraPos.x += 0.05f;
		else if (event.key.keysym.sym == SDLK_w) camera.cameraPos.y+= 0.05f;
		else if (event.key.keysym.sym == SDLK_s) camera.cameraPos.y -= 0.05f;
		else if (event.key.keysym.sym == SDLK_z) camera.cameraPos.z -= 0.05f;
		else if (event.key.keysym.sym == SDLK_x) camera.cameraPos.z += 0.05f;
		else if (event.key.keysym.sym == SDLK_h) camera.cameraPos = xMatrix(0.01, -1) * camera.cameraPos;
		else if (event.key.keysym.sym == SDLK_k) camera.cameraPos = xMatrix(0.01, 1) * camera.cameraPos;
		else if (event.key.keysym.sym == SDLK_u) camera.cameraPos = yMatrix(0.01, -1) * camera.cameraPos;
		else if (event.key.keysym.sym == SDLK_j) camera.cameraPos = yMatrix(0.01, 1) * camera.cameraPos; 
		else if (event.key.keysym.sym == SDLK_UP) camera.cameraOri = yMatrix(0.01, -1) * camera.cameraOri;
		else if (event.key.keysym.sym == SDLK_DOWN) camera.cameraOri = yMatrix(0.01, 1) * camera.cameraOri;
    	else if (event.key.keysym.sym == SDLK_LEFT) camera.cameraOri = xMatrix(0.01, -1) * camera.cameraOri;
		else if (event.key.keysym.sym == SDLK_RIGHT) camera.cameraOri = xMatrix(0.01, 1) * camera.cameraOri;
		else if (event.key.keysym.sym == SDLK_RETURN) camera.toggleMode();
		else if (event.key.keysym.sym == SDLK_1) camera.phong = (camera.phong != true);
	} else if (event.type == SDL_MOUSEBUTTONDOWN) { 
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

std::tuple<std::map<std::string, Colour>, std::map<std::string, TextureMap>> loadPalette(std::string file) {
	std::fstream plt(file);
	std::string line;
	std::string name;
	std::map<std::string, Colour> colourPalette;
	std::map<std::string, TextureMap> texturePalette;
	while (getline(plt, line)) {
		std::vector<std::string> splitLine = split(line, ' ');
		if (splitLine.empty()) {
			continue;
		} else if (splitLine[0] == "newmtl") {
			name = splitLine[1];
		} else if (splitLine[0] == "Kd") {
			colourPalette[name] = Colour(std::stof(splitLine[1])*255, std::stof(splitLine[2])*255, std::stof(splitLine[3])*255);
		} else if (splitLine[0] == "map_Kd") {
			texturePalette[name] = TextureMap(splitLine[1]);
		}
	}
	plt.close();
	return std::make_tuple(colourPalette, texturePalette);
}

std::vector<Face> loadTriangle(std::string file, std::tuple<std::map<std::string, Colour>, std::map<std::string, TextureMap>> palette, float scale) {
	std::fstream obj(file);
	std::vector<Face> faces;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texturevertices;
	std::string line;
	std::map<std::string, Colour> colourPalette = std::get<0>(palette);
	std::map<std::string, TextureMap> texturePalette = std::get<1>(palette);
	Colour colour = Colour(255, 0, 0);
	TextureMap texture;
	while (getline(obj, line)) {
		std::vector<std::string> splitLine = split(line, ' ');
		if (splitLine.empty()) {
			continue;
		} else if (splitLine[0] == "v") {
			vertices.push_back(glm::vec3(std::stof(splitLine[1])*scale, std::stof(splitLine[2])*scale, std::stof(splitLine[3])*scale));
		} else if (splitLine[0] == "vt") {
			texturevertices.push_back(glm::vec2(std::stof(splitLine[1]), std::stof(splitLine[2])));
		} else if (splitLine[0] == "f") {
			std::vector<std::string> vertex1 = split(splitLine[1], '/');
			std::vector<std::string> vertex2 = split(splitLine[2], '/');
			std::vector<std::string> vertex3 = split(splitLine[3], '/');
			ModelTriangle triangle = ModelTriangle(vertices[std::stoi(vertex1[0])-1], vertices[std::stoi(vertex2[0])-1], vertices[std::stoi(vertex3[0])-1], colour);
			Face face;
			if (vertex1[1] != "" && vertex2[1] != "" && vertex3[1] != "") {
				triangle.texturePoints[0] = TexturePoint(texturevertices[std::stoi(vertex1[1])-1][0] * texture.width, texturevertices[std::stoi(vertex1[1])-1][1] * texture.height); 
				triangle.texturePoints[1] = TexturePoint(texturevertices[std::stoi(vertex2[1])-1][0] * texture.width, texturevertices[std::stoi(vertex2[1])-1][1] * texture.height);
				triangle.texturePoints[2] = TexturePoint(texturevertices[std::stoi(vertex3[1])-1][0] * texture.width, texturevertices[std::stoi(vertex3[1])-1][1] * texture.height);
				
				face.texture = texture;
				face.textured = true; 
			}
			triangle.normal = glm::normalize(glm::cross((triangle.vertices[0] - triangle.vertices[1]), (triangle.vertices[0] - triangle.vertices[2])));

			face.triangle = triangle;
			faces.push_back(face);
		} else if (splitLine[0] == "usemtl" && texturePalette.count(splitLine[1])) {
			texture = texturePalette[splitLine[1]];
		} else if (splitLine[0] == "usemtl" && colourPalette.count(splitLine[1])) {
			colour = colourPalette[splitLine[1]];
		}
	}
	obj.close();

	std::vector<glm::vec3> vertexNormals;
	for (int i=0; i<vertices.size(); i++) {
		glm::vec3 total = glm::vec3(0,0,0);
		float number = 0;
		for (int j=0; j<faces.size(); j++) {
			if ((faces[j].triangle.vertices[0] == vertices[i]) || (faces[j].triangle.vertices[1] == vertices[i]) || (faces[j].triangle.vertices[2] == vertices[i])) {
				total += faces[j].triangle.normal;
				number += 1;
			}
		}
		vertexNormals.push_back(glm::normalize(total / number));
	}
	for (int j=0; j<faces.size(); j++) {
		for (int i=0; i<vertices.size(); i++) {
			if (faces[j].triangle.vertices[0] == vertices[i]) faces[j].v0Normal = vertexNormals[i];
			if (faces[j].triangle.vertices[1] == vertices[i]) faces[j].v1Normal = vertexNormals[i];
			if (faces[j].triangle.vertices[2] == vertices[i]) faces[j].v2Normal = vertexNormals[i];
		}
	}
	return faces;
}

void drawWireFrame(DrawingWindow &window, std::vector<Face> model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
	for (int i=0; i<HEIGHT; i++) {
		for (int j=0; j<WIDTH; j++) {
			window.setPixelColour(j, i, 255<<24);
		}	
	}
	CanvasPoint light = projectVertexOntoCanvasPoint(camera, camera.light);
	window.setPixelColour(light.x, light.y, 0xFFFFFFFF);
	std::fill(&zbuf[0][0], &zbuf[0][0] + HEIGHT * WIDTH, 0.0f);

	if (camera.orbit) {
		camera.cameraPos = xMatrix(0.02, -1) * camera.cameraPos;
		lookAt(glm::vec3(0,0,0), camera);
	}
		
	for (int i=0; i<model.size(); i++) {	
		CanvasPoint v0 = projectVertexOntoCanvasPoint(camera, model[i].triangle.vertices[0]);
		CanvasPoint v1 = projectVertexOntoCanvasPoint(camera, model[i].triangle.vertices[1]);
		CanvasPoint v2 = projectVertexOntoCanvasPoint(camera, model[i].triangle.vertices[2]);

		drawLine(window, zbuf, v0, v1, model[i].triangle.colour);
		drawLine(window, zbuf, v1, v2, model[i].triangle.colour);
		drawLine(window, zbuf, v2, v0, model[i].triangle.colour);
	}
	
}

void drawRasterised(DrawingWindow &window, std::vector<Face> model, float (&zbuf)[HEIGHT][WIDTH], cameraClass &camera) {
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
	//std::tuple<std::map<std::string, Colour>, std::map<std::string, TextureMap>> palette = loadPalette("textured-cornell-box.mtl");
	//std::vector<Face> output = loadTriangle("textured-cornell-box.obj", palette, 0.35);
	std::tuple<std::map<std::string, Colour>, std::map<std::string, TextureMap>> palette = loadPalette("textured-cornell-box.mtl");
	std::vector<Face> output = loadTriangle("textured-cornell-box.obj", palette, 0.35);

	cameraClass camera;
	camera.cameraPos = glm::vec3(0.0, 0, 4.0);
	camera.cameraOri = glm::mat3(1.0f);
	camera.orbit = false;
	camera.focalLength = 2.0;
	camera.mode = "wireframe";
	camera.light = glm::vec3(0, 1.0*0.35, 0);
	camera.phong = true;
	//camera.light = glm::vec3(5, 5, 5);
	
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		float start_time = SDL_GetTicks();
		if (window.pollForInputEvents(event)) handleEvent(event, window, zbuf, camera);
		if (camera.mode == "raytraced") {
			drawRaytraced(window, output, zbuf, camera);
		} else if (camera.mode == "rasterised") {
			drawRasterised(window, output, zbuf, camera);
		} else if (camera.mode == "wireframe") {
			drawWireFrame(window, output, zbuf, camera);
		}
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
		//camera.light = xMatrix(0.025, -1) * camera.light;
		std::cout<<"Frame Time: " + std::to_string((SDL_GetTicks() - start_time)/1000) + "s" <<std::endl;
	}
}
