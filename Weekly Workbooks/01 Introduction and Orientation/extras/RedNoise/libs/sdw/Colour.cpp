#include "Colour.h"
#include <utility>

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

