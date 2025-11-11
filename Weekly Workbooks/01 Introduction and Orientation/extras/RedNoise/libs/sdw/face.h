#pragma once

#include <vector>
#include "ModelTriangle.h"
#include "TextureMap.h"
 
class Face {
	public:
		bool textured = false;
        TextureMap texture;
        ModelTriangle triangle;
        glm::vec3 v0Normal;
        glm::vec3 v1Normal;
        glm::vec3 v2Normal;
};