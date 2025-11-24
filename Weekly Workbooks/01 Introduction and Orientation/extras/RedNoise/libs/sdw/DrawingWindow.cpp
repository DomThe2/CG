#include <array>
#include "DrawingWindow.h"
// On some platforms you may need to include <cstring> (if you compiler can't find memset !)

DrawingWindow::DrawingWindow() {}

DrawingWindow::DrawingWindow(int w, int h, bool fullscreen)
    : width(w), height(h), pixelBuffer(w * h),
      headless(false), window(nullptr), renderer(nullptr),
      texture(nullptr), surface(nullptr)
{
    // Try to init video subsystem
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        // If this fails (e.g., no X11/Wayland), go headless
        fprintf(stderr, "SDL video init failed, switching to headless mode: %s\n",
                SDL_GetError());
        headless = true;
    }

    if (!headless) {
        // --- NORMAL MODE ---
        uint32_t flags = SDL_WINDOW_OPENGL;
        if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        window = SDL_CreateWindow("COMS30020",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  width, height, flags);

        if (!window) {
            fprintf(stderr, "SDL window creation failed, switching to headless mode: %s\n",
                    SDL_GetError());
            headless = true;
        }
    }

    if (!headless) {
        // Create renderer
        uint32_t rflags = SDL_RENDERER_SOFTWARE;
        renderer = SDL_CreateRenderer(window, -1, rflags);
        if (!renderer) {
            fprintf(stderr, "SDL renderer failed, switching to headless mode: %s\n",
                    SDL_GetError());
            headless = true;
        }
    }

    if (!headless) {
        SDL_RenderSetLogicalSize(renderer, width, height);

        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STATIC,
                                    width, height);
        if (!texture) {
            fprintf(stderr, "SDL texture failed, switching to headless mode: %s\n",
                    SDL_GetError());
            headless = true;
        }
    }

    // --- HEADLESS MODE SETUP ---
    if (headless) {
        surface = SDL_CreateRGBSurfaceWithFormat(
            0, width, height, 32, SDL_PIXELFORMAT_ARGB8888
        );
        if (!surface) {
            printMessageAndQuit("Headless mode failed: ", SDL_GetError());
        }
    }
}


void DrawingWindow::renderFrame() {
	SDL_UpdateTexture(texture, nullptr, pixelBuffer.data(), width * sizeof(uint32_t));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
}

void DrawingWindow::saveBMP(const std::string &filename) const {
	auto surface = SDL_CreateRGBSurfaceFrom((void *) pixelBuffer.data(), width, height, 32,
	                                        width * sizeof(uint32_t),
	                                        0xFF << 16, 0xFF << 8, 0xFF << 0, 0xFF << 24);
	SDL_SaveBMP(surface, filename.c_str());
}

void DrawingWindow::savePPM(const std::string &filename) const {
	std::ofstream outputStream(filename, std::ofstream::out);
	outputStream << "P6\n";
	outputStream << width << " " << height << "\n";
	outputStream << "255\n";

	for (size_t i = 0; i < width * height; i++) {
		std::array<char, 3> rgb {{
				static_cast<char> ((pixelBuffer[i] >> 16) & 0xFF),
				static_cast<char> ((pixelBuffer[i] >> 8) & 0xFF),
				static_cast<char> ((pixelBuffer[i] >> 0) & 0xFF)
		}};
		outputStream.write(rgb.data(), 3);
	}
	outputStream.close();
}

void DrawingWindow::exitCleanly()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	printMessageAndQuit("Exiting", nullptr);
}

bool DrawingWindow::pollForInputEvents(SDL_Event &event) {
	if (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) exitCleanly();
		else if ((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_ESCAPE)) exitCleanly();
		else if ((event.type == SDL_WINDOWEVENT) && (event.window.event == SDL_WINDOWEVENT_CLOSE)) exitCleanly();
		SDL_Event dummy;
		// Clear the event queue by getting all available events
		// This seems like bad practice (because it will skip some events) however preventing backlog is paramount !
		while (SDL_PollEvent(&dummy));
		return true;
	}
	return false;
}

void DrawingWindow::setPixelColour(size_t x, size_t y, uint32_t colour) {
	if ((x >= width) || (y >= height)) {
		std::cout << x << "," << y << " not on visible screen area" << std::endl;
	} else pixelBuffer[(y * width) + x] = colour;
}

uint32_t DrawingWindow::getPixelColour(size_t x, size_t y) {
	if ((x >= width) || (y >= height)) {
		std::cout << x << "," << y << " not on visible screen area" << std::endl;
		return -1;
	} else return pixelBuffer[(y * width) + x];
}

void DrawingWindow::clearPixels() {
	std::fill(pixelBuffer.begin(), pixelBuffer.end(), 0);
}

void printMessageAndQuit(const std::string &message, const char *error) {
	if (error == nullptr) {
		std::cout << message << std::endl;
		exit(0);
	} else {
		std::cout << message << " " << error << std::endl;
		exit(1);
	}
}
