#include "Colour.h"
#include <utility>
#include <glm/glm.hpp>

Colour::Colour() = default;
Colour::Colour(int r, int g, int b) : red(r), green(g), blue(b) {}
Colour::Colour(std::string n, int r, int g, int b) :
		name(std::move(n)),
		red(r), green(g), blue(b) {}


Colour Colour::operator*(float s) const {
    return Colour(
        static_cast<int>(red   * s),
        static_cast<int>(green * s),
        static_cast<int>(blue  * s)
    );
}

Colour Colour::operator*(glm::vec3 v) const {
    return Colour(
        static_cast<int>(red   * v[0]),
        static_cast<int>(green * v[1]),
        static_cast<int>(blue  * v[2])
    );
}

Colour Colour::operator+(glm::vec3 v) const {
    return Colour(
        static_cast<int>(red   + v[0]),
        static_cast<int>(green + v[1]),
        static_cast<int>(blue  + v[2])
    );
}

Colour Colour::operator+(const Colour& other) const {
    return Colour(
        red   + other.red,
        green + other.green,
        blue  + other.blue
    );
}

Colour Colour::operator*(const Colour& other) const {
    return Colour(
        red   * other.red,
        green * other.green,
        blue  * other.blue
    );
}

std::ostream &operator<<(std::ostream &os, const Colour &colour) {
	os << colour.name << " ["
	   << colour.red << ", "
	   << colour.green << ", "
	   << colour.blue << "]";
	return os;
}

