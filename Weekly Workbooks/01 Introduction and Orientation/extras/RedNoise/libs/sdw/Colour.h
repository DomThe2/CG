#pragma once

#include <iostream>
#include <glm/glm.hpp>

struct Colour {
	std::string name;
	int red{};
	int green{};
	int blue{};
	Colour();
	Colour(int r, int g, int b);
	Colour(std::string n, int r, int g, int b);
	
	Colour operator*(glm::vec3 v) const;
	Colour operator+(glm::vec3 v) const;
	Colour operator*(float s) const;
	Colour operator+(const Colour& other) const;
	Colour operator*(const Colour& other) const;
};

std::ostream &operator<<(std::ostream &os, const Colour &colour);

