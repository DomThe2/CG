#pragma once

#include <vector>
#include "ModelTriangle.h"
#include "TextureMap.h"
#include "Colour.h"
 
class Face {
	public:
		bool textured = false;
        bool mirror = false;
        bool phong = false;
        float opacity;
        float fuzziness;
        int specularExponent;
        glm::vec3 transmission;
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 ambient;
        TextureMap texture;
        ModelTriangle triangle;
        glm::vec3 v0Normal;
        glm::vec3 v1Normal;
        glm::vec3 v2Normal;

    Colour getColour() {
        return Colour(diffuse[0]*255.0f, diffuse[1]*255.0f, diffuse[2]*255.0f);
    }
}; 