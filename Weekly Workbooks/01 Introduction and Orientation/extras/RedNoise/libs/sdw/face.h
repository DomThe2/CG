#pragma once

#include <vector>
#include "ModelTriangle.h"
#include "TextureMap.h"
#include "Colour.h"
#include "RayTriangleIntersection.h"
#include "face.h"
#include "Utils.h"

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

    // convert my float vec3 colour to the colour class
    Colour getColour() {
        return Colour(diffuse[0]*255.0f, diffuse[1]*255.0f, diffuse[2]*255.0f);
    }

    // get the barycentric coordinates of the position on the INTERSECTED (not my) triangle 
    glm::vec3 getBarycentricOfIntersection(RayTriangleIntersection intersection) {
        return convertToBarycentricCoordinates(intersection.intersectedTriangle.vertices[0], intersection.intersectedTriangle.vertices[1], 
                                          intersection.intersectedTriangle.vertices[2], intersection.intersectionPoint);
    }
    
    // get the phong normal of the position on the INTERSECTED (not my) triangle
    glm::vec3 getphongNormal(RayTriangleIntersection intersection, std::vector<Face>& model) {
        glm::vec3 barycentric = model[intersection.triangleIndex].getBarycentricOfIntersection(intersection);
        glm::vec3 normal = barycentric.z * model[intersection.triangleIndex].v0Normal 
            + barycentric.x * model[intersection.triangleIndex].v1Normal 
            + barycentric.y * model[intersection.triangleIndex].v2Normal;
        return glm::normalize(normal);
    }

    // get the normal of the position on the INTERSECTED (not my) triangle
    glm::vec3 getNormal(RayTriangleIntersection intersection, std::vector<Face>& model) {
        // if the face has phong shading enabled, get the phong normal
        if (model[intersection.triangleIndex].phong) return getphongNormal(intersection, model);
        // otherwise just use the standard normal
        else return intersection.intersectedTriangle.normal;
    }
}; 