#include <iostream>
#include <SFML/Graphics.hpp>

#define SCREEN_RESOLUTION 640
#define SIMULATION_RESOLUTION 256
#define CANVAS_SCALE_FACTOR (float)SCREEN_RESOLUTION / (float)SIMULATION_RESOLUTION

// TEMP: Random colour code is only for demonstration of the canvas rendering technique
#define RAND_FLOAT (float)(rand()) / (float)(rand())
#define LERP(v0, v1, t) \
	v0 + t * (v1 - v0)

// TEMP: Random colour code is only for demonstration of the canvas rendering technique
sf::Color PickRandomColour()
{
	sf::Color retVal = sf::Color();

	retVal.r = LERP(0, 255, RAND_FLOAT);
	retVal.g = LERP(0, 255, RAND_FLOAT);
	retVal.b = LERP(0, 255, RAND_FLOAT);
	retVal.a = 255;

	return retVal;
}

int main()
{
	sf::RenderWindow wWindow(sf::VideoMode(SCREEN_RESOLUTION, SCREEN_RESOLUTION), "Tinderbox");
	sf::Event eWinEvent;

	while (wWindow.isOpen())
	{
		// ---- RENDER BEGINS ----
		wWindow.clear();

		// Create our canvas image
		// We need an sf::Image as that provides the easiest access to an array of pixel data
		// This single image is then scaled up to fill the screen
		sf::Image imCanvas;
		imCanvas.create(SIMULATION_RESOLUTION, SIMULATION_RESOLUTION, sf::Color::White);

		// Draw each pixel of the canvas
		for (uint16_t x = 0; x < SIMULATION_RESOLUTION; ++x)
		{
			for (uint16_t y = 0; y < SIMULATION_RESOLUTION; ++y)
			{
				// TEMP: For now, we just pick a random colour
				imCanvas.setPixel(x, y, PickRandomColour());
			}
		}

		// Convert image to texture to be applied to a sprite
		sf::Texture tCanvasTexture;
		tCanvasTexture.loadFromImage(imCanvas);

		// Create a sprite, apply the canvas texture
		sf::Sprite sCanvasSprite;
		sCanvasSprite.setTexture(tCanvasTexture);
		sCanvasSprite.setScale(CANVAS_SCALE_FACTOR, CANVAS_SCALE_FACTOR);

		// Draw our canvas, and render the window
		wWindow.draw(sCanvasSprite);
		wWindow.display();
		// ---- RENDER ENDS ----

		while (wWindow.pollEvent(eWinEvent))
		{
			switch (eWinEvent.type)
			{
				case sf::Event::Closed:
					wWindow.close();
					break;
			}
		}
	}
}


