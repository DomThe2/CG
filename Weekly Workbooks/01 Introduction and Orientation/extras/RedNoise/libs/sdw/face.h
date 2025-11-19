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
        int specularExponent;
        Colour transmission;
        Colour diffuse;
        Colour specular;
        Colour ambient;
        TextureMap texture;
        ModelTriangle triangle;
        glm::vec3 v0Normal;
        glm::vec3 v1Normal;
        glm::vec3 v2Normal;
}; 