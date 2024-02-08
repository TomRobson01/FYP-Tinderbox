#include <iostream>
//#include <windows.h>
#include "ParticleSimulation.h"

#define SCREEN_RESOLUTION 640
#define CANVAS_SCALE_FACTOR ((float)SCREEN_RESOLUTION / (float)simulationResolution)

namespace Painting
{
	sf::Vector2i WorldToSimulationSpaceCoords(const sf::Vector2i avInVal)
	{
		sf::Vector2i retval = avInVal;
		retval.x /= CANVAS_SCALE_FACTOR;
		retval.y /= CANVAS_SCALE_FACTOR;
		return retval;
	}

	PARTICLE_TYPE pCurrentlyPaintingParticle = PARTICLE_TYPE::SOLID;
	bool bPainting = false;
	bool bErasing = false;
	bool bIgniting = false;
};

int main()
{
	sf::RenderWindow wWindow(sf::VideoMode(SCREEN_RESOLUTION, SCREEN_RESOLUTION), "Tinderbox");
	sf::Event eWinEvent;

	// Setup performance metrics output text objects
	sf::Font fFont;
	fFont.loadFromFile("Assets\\Fonts\\Roboto.ttf");

	sf::Text tFPSCount;
	tFPSCount.setFont(fFont);
	tFPSCount.setString("Hello world!");
	tFPSCount.setScale(0.4f, 0.4f);

	sf::Text tActiveParticlesCount;
	tActiveParticlesCount.setFont(fFont);
	tActiveParticlesCount.setString("Hello world!");
	tActiveParticlesCount.setPosition(0, 16);
	tActiveParticlesCount.setScale(0.4f, 0.4f);

	sf::Text tTotalParticlesCount;
	tTotalParticlesCount.setFont(fFont);
	tTotalParticlesCount.setString("Hello world!");
	tTotalParticlesCount.setPosition(0, 32);
	tTotalParticlesCount.setScale(0.4f, 0.4f);
	// -------------------

	clock_t currentTicks;
	clock_t deltaTicks; 
	int ifps = 60;

	while (wWindow.isOpen())
	{
		currentTicks = clock();

		// Display important profiling information
		const int iParticleCount = ParticleSimulation::QInstance().QParticleCount();
		const int iactiveParticles = ParticleSimulation::QInstance().QActiveParticleCount();
		tFPSCount.setString(std::to_string(ifps) + " FPS");
		tActiveParticlesCount.setString(std::to_string(iactiveParticles) + " Active Particles");
		tTotalParticlesCount.setString(std::to_string(iParticleCount) + " Total Particles");

		// ---- RENDER BEGINS ----
		wWindow.clear();

		// Create our canvas image
		// We need an sf::Image as that provides the easiest access to an array of pixel data
		// This single image is then scaled up to fill the screen
		sf::Image imCanvas;
		imCanvas.create(simulationResolution, simulationResolution, sf::Color::Black);

		ParticleSimulation::QInstance().Tick(imCanvas);

		// Convert image to texture to be applied to a sprite
		sf::Texture tCanvasTexture;
		tCanvasTexture.loadFromImage(imCanvas);

		// Create a sprite, apply the canvas texture
		sf::Sprite sCanvasSprite;
		sCanvasSprite.setTexture(tCanvasTexture);
		sCanvasSprite.setScale(CANVAS_SCALE_FACTOR, CANVAS_SCALE_FACTOR);

		// Draw our canvas, and render the window
		wWindow.draw(sCanvasSprite);
		wWindow.draw(tFPSCount);
		wWindow.draw(tActiveParticlesCount);
		wWindow.draw(tTotalParticlesCount);
		wWindow.display();
		// ---- RENDER ENDS ----

		while (wWindow.pollEvent(eWinEvent))
		{
			switch (eWinEvent.type)
			{
				// Application events
				case sf::Event::Closed:
					wWindow.close();
					break;

				// Input events
				// Toggle paint mode
				case sf::Event::MouseButtonPressed:
					if (eWinEvent.mouseButton.button == sf::Mouse::Button::Left && !Painting::bErasing)
					{
						Painting::bPainting = true;
					}
					if (eWinEvent.mouseButton.button == sf::Mouse::Button::Right && !Painting::bPainting)
					{
						Painting::bErasing = true;
						std::cout << "Entered erase mode" << std::endl;
					}
					break;
				case sf::Event::MouseButtonReleased:
					if (eWinEvent.mouseButton.button == sf::Mouse::Button::Left && !Painting::bErasing)
					{
						Painting::bPainting = false;
					}
					if (eWinEvent.mouseButton.button == sf::Mouse::Button::Right && !Painting::bPainting)
					{
						Painting::bErasing = false;
					}
					break;

				// Keyboard events
				case sf::Event::KeyPressed:
				{
					switch (eWinEvent.key.code)
					{
						case sf::Keyboard::R:
							ParticleSimulation::QInstance().ResetSimulation();
							break;

						// Set paint materials
						case sf::Keyboard::Num1:
							std::cout << "Painting with solid" << std::endl;
							Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::SOLID;
							Painting::bIgniting = false;
							break;
						case sf::Keyboard::Num2:
							std::cout << "Painting with powder" << std::endl;
							Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::POWDER;
							Painting::bIgniting = false;
							break;
						case sf::Keyboard::Num3:
							std::cout << "Painting with liquid" << std::endl;
							Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::LIQUID;
							Painting::bIgniting = false;
							break;
						case sf::Keyboard::Num4:
							std::cout << "Painting with gas" << std::endl;
							Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::GAS;
							Painting::bIgniting = false;
							break;
						case sf::Keyboard::Num5:
							std::cout << "Painting with fire" << std::endl;
							Painting::bIgniting = true;
							break;
					}
					break;
				}
			}
		}

		// If in painting mode, create particle at the current mouse position
		if (Painting::bPainting)
		{
			const sf::Vector2i mousePos = Painting::WorldToSimulationSpaceCoords(sf::Mouse::getPosition(wWindow));
			if (Painting::bIgniting)
			{
				ParticleSimulation::QInstance().IgniteParticle(mousePos.x, mousePos.y);
			}
			else
			{
				ParticleSimulation::QInstance().SpawnParticle(mousePos.x, mousePos.y, Painting::pCurrentlyPaintingParticle);
			}
		}
		if (Painting::bErasing)
		{
			const sf::Vector2i mousePos = Painting::WorldToSimulationSpaceCoords(sf::Mouse::getPosition(wWindow));
			ParticleSimulation::QInstance().DestroyParticle(mousePos.x, mousePos.y);
		}

		// FPS calculation
		deltaTicks = clock() - currentTicks;
		ifps = deltaTicks > 0 ? CLOCKS_PER_SEC / deltaTicks : 0;
	}
}


