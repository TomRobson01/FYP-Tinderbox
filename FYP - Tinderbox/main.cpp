#include <iostream>
#include <thread>
#include "ParticleSimulation.h"

#define SCREEN_RESOLUTION 900
#define CANVAS_SCALE_FACTOR ((float)SCREEN_RESOLUTION / (float)simulationResolution)

#define SCREEN_CLEAR_COLOUR sf::Color(13,14,15,255)

#define DEFINE_PARTICLE_MAPPING(NUM, TYPE, MESSAGE) \
	case NUM:\
		std::cout << MESSAGE << std::endl; \
		Painting::pCurrentlyPaintingParticle = TYPE; \
		Painting::bIgniting = false; \
		break; \

#define DEFINE_DEBUG_STAT_TEXT(NAME, XPOS, YPOS, INITIAL_TEXT) \
	sf::Text NAME; \
	NAME.setFont(fFont); \
	NAME.setString(INITIAL_TEXT); \
	NAME.setPosition(XPOS, YPOS); \
	NAME.setScale(0.4f, 0.4f); \

#define SET_DEBUG_STAT_TEXT_VAL(NAME, VAL, TEXT) \
	NAME.setString(std::to_string(VAL) + " " + TEXT);

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
	int iBrushSize = 3;
};

bool bDoOnce = false;

int main()
{
	// Help message to better explain program use
	std::cout << "====================" << std::endl;
	std::cout << "Welcome to Tinderbox" << std::endl;
	std::cout << "====================" << std::endl;
	std::cout << "\nControls ---------" << std::endl;
	std::cout << "1-0: Element bindings" << std::endl;
	std::cout << "F1: Show performance metrics" << std::endl;
	std::cout << "F2: Show chunk boundaries" << std::endl;
	std::cout << "F10: Brush size 1" << std::endl;
	std::cout << "F11: Brush size 3" << std::endl;
	std::cout << "F12: Brush size 5" << std::endl;
	std::cout << "\n" << std::endl;

	sf::RenderWindow wWindow(sf::VideoMode(SCREEN_RESOLUTION, SCREEN_RESOLUTION), "Tinderbox");
	sf::Event eWinEvent;

	// Setup performance metrics output text objects
	sf::Font fFont;
	fFont.loadFromFile("Assets\\Fonts\\Roboto.ttf");

	DEFINE_DEBUG_STAT_TEXT(FPSCount, 4, 0, "");
	DEFINE_DEBUG_STAT_TEXT(FrameMS, 4, 16, "");
	DEFINE_DEBUG_STAT_TEXT(TitleStats, 4, 32, "--- STATS ---");
	DEFINE_DEBUG_STAT_TEXT(ActiveParticlesCount, 8, 48, "");
	DEFINE_DEBUG_STAT_TEXT(TotalParticlesCount, 8, 64, "");
	DEFINE_DEBUG_STAT_TEXT(ParticleVisitsCountTotal, 8, 80, "");
	DEFINE_DEBUG_STAT_TEXT(ParticleVisitsCountPreChunk, 24, 96, "");
	DEFINE_DEBUG_STAT_TEXT(ParticleVisitsCountChunkTick, 24, 112, "");
	DEFINE_DEBUG_STAT_TEXT(ParticleVisitsCountWakeChunk, 24, 128, "");
	DEFINE_DEBUG_STAT_TEXT(ParticleVisitsCountAllowUpdate, 24, 144, "");
	DEFINE_DEBUG_STAT_TEXT(ParticleVisitsCountExpiredCleanup, 24, 160, "");
	DEFINE_DEBUG_STAT_TEXT(ChunkVisitsCount, 8, 176, "");
	DEFINE_DEBUG_STAT_TEXT(BurningParticles, 8, 192, "");
	// -------------------

	clock_t currentTicks;
	clock_t deltaTicks = clock();
	int ifps = 60;

	while (wWindow.isOpen())
	{
		currentTicks = clock();

		// Display important profiling information
		const int iParticleCount = ParticleSimulation::QInstance().QParticleCount();
		const int iactiveParticles = ParticleSimulation::QInstance().QActiveParticleCount();
		const int iparticleVisitsTotal = ParticleSimulation::QInstance().QParticleVisitsTotal();
		const int iparticleVisitsPreChunk = ParticleSimulation::QInstance().QParticleVisitsPreChunk();
		const int iparticleVisitsChunkTick = ParticleSimulation::QInstance().QParticleVisitsChunkTick();
		const int iparticleVisitsWakeChunk = ParticleSimulation::QInstance().QParticleVisitsWakeChunk();
		const int iparticleVisitsAllowUpdates = ParticleSimulation::QInstance().QParticleVisitsAllowUpdate();
		const int iparticleVisitsExpiredCleanup = ParticleSimulation::QInstance().QParticleVisitsExpiredCleanup();
		const int ichunkVisits = ParticleSimulation::QInstance().QChunkVisits();
		const int iBurningParticles = ParticleSimulation::QInstance().QBurningParticles();

		SET_DEBUG_STAT_TEXT_VAL(FPSCount,							ifps,							"FPS");
		SET_DEBUG_STAT_TEXT_VAL(FrameMS,							deltaTicks,						"MS");
		SET_DEBUG_STAT_TEXT_VAL(ActiveParticlesCount,				iactiveParticles,				"Active Particles");
		SET_DEBUG_STAT_TEXT_VAL(TotalParticlesCount,				iParticleCount,					"Total Particles");
		SET_DEBUG_STAT_TEXT_VAL(ParticleVisitsCountTotal,			iparticleVisitsTotal,			"Pixel Visits");
		SET_DEBUG_STAT_TEXT_VAL(ParticleVisitsCountPreChunk,		iparticleVisitsPreChunk,		"Pre-Chunk");
		SET_DEBUG_STAT_TEXT_VAL(ParticleVisitsCountChunkTick,		iparticleVisitsChunkTick,		"Chunk Tick");
		SET_DEBUG_STAT_TEXT_VAL(ParticleVisitsCountWakeChunk,		iparticleVisitsWakeChunk,		"Wake Chunk");
		SET_DEBUG_STAT_TEXT_VAL(ParticleVisitsCountAllowUpdate,		iparticleVisitsAllowUpdates,	"Allow Updates");
		SET_DEBUG_STAT_TEXT_VAL(ParticleVisitsCountExpiredCleanup,	iparticleVisitsExpiredCleanup,	"Expired Cleanup");
		SET_DEBUG_STAT_TEXT_VAL(ChunkVisitsCount,					ichunkVisits,					"Chunk Visits");
		SET_DEBUG_STAT_TEXT_VAL(BurningParticles,					iBurningParticles,				"Burning Particles");

		// ---- RENDER BEGINS ----
		wWindow.clear();

		// Create our canvas image
		// We need an sf::Image as that provides the easiest access to an array of pixel data
		// This single image is then scaled up to fill the screen
		sf::Image imCanvas;
		imCanvas.create(simulationResolution, simulationResolution, SCREEN_CLEAR_COLOUR);

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
		if (DebugToggles::QInstance().bShowPerformanceStats)
		{
			wWindow.draw(FPSCount);
			wWindow.draw(FrameMS);
			wWindow.draw(TitleStats);
			wWindow.draw(ActiveParticlesCount);
			wWindow.draw(TotalParticlesCount);
			wWindow.draw(ParticleVisitsCountTotal);
			wWindow.draw(ParticleVisitsCountPreChunk);
			wWindow.draw(ParticleVisitsCountChunkTick);
			wWindow.draw(ParticleVisitsCountWakeChunk);
			wWindow.draw(ParticleVisitsCountAllowUpdate);
			wWindow.draw(ParticleVisitsCountExpiredCleanup);
			wWindow.draw(ChunkVisitsCount);
			wWindow.draw(BurningParticles);
		}
		if (DebugToggles::QInstance().bShowChunkBoundaries)
		{
			for (int x = 0; x < SCREEN_RESOLUTION; x += (SCREEN_RESOLUTION / chunkCount))
			{
				sf::Vertex vLine[2];
				vLine[0].position = sf::Vector2f(x, 0);
				vLine[0].color = sf::Color::Red;
				vLine[1].position = sf::Vector2f(x, SCREEN_RESOLUTION);
				vLine[1].color = sf::Color::Red;
				wWindow.draw(vLine, 2, sf::Lines);
			}
		}
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
						case sf::Keyboard::F1:
							DebugToggles::QInstance().bShowPerformanceStats = !DebugToggles::QInstance().bShowPerformanceStats;
							break;
						case sf::Keyboard::F2:
							DebugToggles::QInstance().bShowChunkBoundaries = !DebugToggles::QInstance().bShowChunkBoundaries;
							break;

						case sf::Keyboard::F10:
							Painting::iBrushSize = 1;
							std::cout << "Brush size: 1\n";
							break;
						case sf::Keyboard::F11:
							Painting::iBrushSize = 3;
							std::cout << "Brush size: 3\n";
							break;
						case sf::Keyboard::F12:
							Painting::iBrushSize = 5;
							std::cout << "Brush size: 5\n";
							break;

						case sf::Keyboard::R:
							ParticleSimulation::QInstance().ResetSimulation();
							break;

						// Set paint materials
						case sf::Keyboard::Num0:
							std::cout << "Painting with fire" << std::endl;
							Painting::bIgniting = true;
							break;

						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num1, PARTICLE_TYPE::WOOD, "Painting with wood");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num2, PARTICLE_TYPE::ROCK, "Painting with rock");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num3, PARTICLE_TYPE::METAL, "Painting with metal");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num4, PARTICLE_TYPE::SAND, "Painting with sand");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num5, PARTICLE_TYPE::COAL, "Painting with coal");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num6, PARTICLE_TYPE::LEAVES, "Painting with leaves");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num7, PARTICLE_TYPE::STEAM, "Painting with steam");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num8, PARTICLE_TYPE::SMOKE, "Painting with smoke");
						DEFINE_PARTICLE_MAPPING(sf::Keyboard::Num9, PARTICLE_TYPE::WATER, "Painting with water");
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
				if (Painting::iBrushSize > 1)
				{
					for (int x = mousePos.x - Painting::iBrushSize; x < mousePos.x + Painting::iBrushSize; ++x)
					{
						for (int y = mousePos.y - Painting::iBrushSize; y < mousePos.y + Painting::iBrushSize; ++y)
						{
							ParticleSimulation::QInstance().SpawnParticle(x, y, Painting::pCurrentlyPaintingParticle);
						}
					}
				}
				else
				{
					ParticleSimulation::QInstance().SpawnParticle(mousePos.x, mousePos.y, Painting::pCurrentlyPaintingParticle);
				}
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


