#include <algorithm>
#include <sstream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <iterator>
#include "DrawingWindow.h"
#include "Utils.h"
#include "Colour.h"
#include "face.h"
#include <map>
#include <string>

#include "CanvasPoint.h"
#include "parser.h"

struct material {
	bool textured = false;
	float opacity;
	int specularExponent;
	glm::vec3 transmission;
	glm::vec3 diffuse;
	glm::vec3 specular;
	glm::vec3 ambient;
	TextureMap texture;
};

void storePhotonMap(std::vector<photon> lightMap, std::string filename) {
	std::ofstream file(filename);
	for (int i=0; i<lightMap.size(); i++) {
		file << lightMap[i].location[0] << " " << lightMap[i].location[1] << " " << lightMap[i].location[2] 
		<< " " << lightMap[i].incidence[0] << " " << lightMap[i].incidence[1] << " " << lightMap[i].incidence[2] 
		<< " " << lightMap[i].power[0] << " " << lightMap[i].power[1] << " " << lightMap[i].power[2] << "\n";
	}
	file.close();
}

std::vector<photon> getPhotonMap(std::string filename) {
	std::fstream file(filename);
	std::string line;
	std::vector<photon> photonMap;
	photon p;
	while (getline(file, line)) {
		std::vector<std::string> splitLine = split(line, ' ');
		if (splitLine.empty()) {
			continue;
		} else {
			p.location = glm::vec3(std::stof(splitLine[0]), std::stof(splitLine[1]), std::stof(splitLine[2]));
			p.incidence = glm::vec3(std::stof(splitLine[3]), std::stof(splitLine[4]), std::stof(splitLine[5]));
			p.power = glm::vec3(std::stof(splitLine[6]), std::stof(splitLine[7]), std::stof(splitLine[8]));
			photonMap.push_back(p);
		}
	}
	file.close();
	return photonMap;
}

std::map<std::string, material> loadPalette(std::string file) {
	std::fstream plt(file);
	std::string line;
	std::string name = "";
	material mat;
	std::map<std::string, material> palette;
	while (getline(plt, line)) {
		std::vector<std::string> splitLine = split(line, ' ');
		if (splitLine.empty()) {
			continue;
		} else if (splitLine[0] == "newmtl") {
			if (name != "") palette[name] = mat;
			mat.textured = false;
			name = splitLine[1];
		} else if (splitLine[0] == "Kd") {
			mat.diffuse = glm::vec3(std::stof(splitLine[1]), std::stof(splitLine[2]), std::stof(splitLine[3]));
		} else if (splitLine[0] == "Ks") {
			mat.specular = glm::vec3(std::stof(splitLine[1]), std::stof(splitLine[2]), std::stof(splitLine[3]));
		} else if (splitLine[0] == "Ka") {
			mat.ambient = glm::vec3(std::stof(splitLine[1]), std::stof(splitLine[2]), std::stof(splitLine[3]));
		} else if (splitLine[0] == "Tf") {
			mat.transmission = glm::vec3(std::stof(splitLine[1]), std::stof(splitLine[2]), std::stof(splitLine[3]));
		} else if (splitLine[0] == "Ns") {
			mat.specularExponent = std::stoi(splitLine[1]);
		} else if (splitLine[0] == "d") {
			mat.opacity = std::stof(splitLine[1]);
		} else if (splitLine[0] == "map_Kd") {
			mat.texture = TextureMap(splitLine[1]);
			mat.textured = true;
		}
	}
	palette[name] = mat;
	plt.close();
	return palette;
}

std::vector<Face> loadTriangle(std::string file, float scale) {
	std::fstream obj(file);
	std::vector<Face> faces;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texturevertices;
	std::string line;
	Colour colour = Colour(255, 0, 0);
	bool mirrored = false;
	bool phong = false;
	std::map<std::string, material> palette;
	material mat;
	while (getline(obj, line)) {
		std::vector<std::string> splitLine = split(line, ' ');
		if (splitLine.empty()) {
			continue;
		} else if (splitLine[0] == "mtllib") {
			palette = loadPalette(splitLine[1]);
		} else if (splitLine[0] == "v") {
			vertices.push_back(glm::vec3(std::stof(splitLine[1])*scale, std::stof(splitLine[2])*scale, std::stof(splitLine[3])*scale));
		} else if (splitLine[0] == "vt") {
			texturevertices.push_back(glm::vec2(std::stof(splitLine[1]), std::stof(splitLine[2])));
		} else if (splitLine[0] == "f") {
			std::vector<std::string> vertex1 = split(splitLine[1], '/');
			std::vector<std::string> vertex2 = split(splitLine[2], '/');
			std::vector<std::string> vertex3 = split(splitLine[3], '/');
			Colour colour = Colour(mat.diffuse[0]*255.0f, mat.diffuse[1]*255.0f, mat.diffuse[2]*255.0f);
			ModelTriangle triangle = ModelTriangle(vertices[std::stoi(vertex1[0])-1], vertices[std::stoi(vertex2[0])-1], vertices[std::stoi(vertex3[0])-1], colour);
			Face face;
			if (mat.textured) {
				triangle.texturePoints[0] = TexturePoint(texturevertices[std::stoi(vertex1[1])-1][0] * mat.texture.width, texturevertices[std::stoi(vertex1[1])-1][1] * mat.texture.height); 
				triangle.texturePoints[1] = TexturePoint(texturevertices[std::stoi(vertex2[1])-1][0] * mat.texture.width, texturevertices[std::stoi(vertex2[1])-1][1] * mat.texture.height);
				triangle.texturePoints[2] = TexturePoint(texturevertices[std::stoi(vertex3[1])-1][0] * mat.texture.width, texturevertices[std::stoi(vertex3[1])-1][1] * mat.texture.height);
				face.texture = mat.texture;
				face.textured = true; 
			}
			
			triangle.normal = glm::normalize(glm::cross((triangle.vertices[0] - triangle.vertices[1]), (triangle.vertices[0] - triangle.vertices[2])));
			face.mirror = mirrored;
			face.opacity = mat.opacity;
			face.specularExponent = mat.specularExponent;
			face.transmission = mat.transmission;
			face.diffuse = mat.diffuse;
			face.specular = mat.specular;
			face.ambient = mat.ambient;
			face.triangle = triangle;
			face.phong = phong;
			faces.push_back(face);
		} else if (splitLine[0] == "s") {
			if (splitLine[1] == "1") phong = true;
			else phong = false;
		} else if (splitLine[0] == "usemtl" && (splitLine[1]=="Mirror")) {
			mat = palette.at(splitLine[1]);
			mirrored = true; // TEMP
		} else if (splitLine[0] == "usemtl") {
			mat = palette.at(splitLine[1]);
			mirrored = false;
		} 
	}
	obj.close();

	std::vector<glm::vec3> vertexNormals;
	for (int i=0; i<vertices.size(); i++) {
		glm::vec3 total = glm::vec3(0,0,0);
		float number = 0;
		for (int j=0; j<faces.size(); j++) {
			if ((glm::length(faces[j].triangle.vertices[0] - vertices[i]) < 1e-6f) || (glm::length(faces[j].triangle.vertices[1] - vertices[i]) < 1e-6f) || (glm::length(faces[j].triangle.vertices[2] - vertices[i]) < 1e-6f)) {
				total += faces[j].triangle.normal;
				number += 1;
			}
		}
		vertexNormals.push_back(glm::normalize(total / number));
	}
	for (int j=0; j<faces.size(); j++) {
		if (faces[j].phong) {
			for (int i=0; i<vertices.size(); i++) {
				if (glm::length(faces[j].triangle.vertices[0] - vertices[i]) < 1e-6f) faces[j].v0Normal = vertexNormals[i];
				if (glm::length(faces[j].triangle.vertices[1] - vertices[i]) < 1e-6f) faces[j].v1Normal = vertexNormals[i];
				if (glm::length(faces[j].triangle.vertices[2] - vertices[i]) < 1e-6f) faces[j].v2Normal = vertexNormals[i];
			}
		}
	}
	return faces;
}