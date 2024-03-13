#include <iostream>
#include <thread>

#include <SFML/Graphics.hpp>

#include "ParticleSimulation.h"
#include "PerformanceReporter.h"
#include "SimulationSerializer.h"
#include "UIButton.h"

#define SCREEN_RESOLUTION 900
#define CANVAS_SCALE_FACTOR ((float)SCREEN_RESOLUTION / (float)simulationResolution)

#define SCREEN_CLEAR_COLOUR sf::Color(13,14,15,255)

#define UI_TOOLBAR_X_PADDING 16
#define UI_TOOLBAR_Y_PADDING 38
#define UI_TOOLBAR_Y_EDGE_PADDING 16

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


enum class TOOLBAR_BUTTONS : uint8_t
{
	PAINT,
	IGNITE,
	WOOD,
	STONE,
	METAL,
	SAND,
	COAL,
	LEAVES,
	STEAM,
	SMOKE,
	WATER,
	COUNT
};
UIButton* ToolBar[static_cast<int>(TOOLBAR_BUTTONS::COUNT)];

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
	std::cout << "F9: Brush size 1" << std::endl;
	std::cout << "F10: Brush size 3" << std::endl;
	std::cout << "F11: Brush size 5" << std::endl;
	std::cout << "F12: Brush size 7" << std::endl;
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

	// UI Setup
	for (int i = 0; i < static_cast<int>(TOOLBAR_BUTTONS::COUNT); ++i)
	{
		sf::Vector2f fStartPos = sf::Vector2f(UI_TOOLBAR_X_PADDING, UI_TOOLBAR_Y_EDGE_PADDING + UI_TOOLBAR_Y_PADDING * i);
		std::string sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Empty_Base.png";
		std::string sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Empty_Hovered.png";

		switch (static_cast<TOOLBAR_BUTTONS>(i))
		{
			// Input tools
		case TOOLBAR_BUTTONS::PAINT:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Paint_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Paint_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::IGNITE:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Ignite_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Ignite_Hovered.png";
			break;
			// Elements
		case TOOLBAR_BUTTONS::WOOD:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Wood_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Wood_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::STONE:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Stone_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Stone_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::METAL:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Metal_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Metal_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::SAND:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Sand_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Sand_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::COAL:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Coal_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Coal_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::LEAVES:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Leaves_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Leaves_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::STEAM:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Steam_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Steam_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::SMOKE:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Smoke_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Smoke_Hovered.png";
			break;
		case TOOLBAR_BUTTONS::WATER:
			sBasePath = "Assets\\Sprites\\UI\\TB_Icon_Water_Base.png";
			sHoveredPath = "Assets\\Sprites\\UI\\TB_Icon_Water_Hovered.png";
			break;

		default:
			break;
		}

		ToolBar[i] = new UIButton();
		ToolBar[i]->Initialize(fStartPos, sBasePath, sHoveredPath);
	}

	clock_t currentTicks;
	clock_t deltaTicks = clock();
	int ifps = 60;
	int iTicksPerPerfCapture = 100;
	int iTicksUntilPerfCapture = iTicksPerPerfCapture;

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

		// TICKS
		// MAIN TICK
		ParticleSimulation::QInstance().Tick(imCanvas);

		// UI TICK
		for (int i = 0; i < static_cast<int>(TOOLBAR_BUTTONS::COUNT); ++i)
		{
			ToolBar[i]->Tick(sf::Mouse::getPosition(wWindow));
		}

		// Convert image to texture to be applied to a sprite
		sf::Texture tCanvasTexture;
		tCanvasTexture.loadFromImage(imCanvas);

		// Create a sprite, apply the canvas texture
		sf::Sprite sCanvasSprite;
		sCanvasSprite.setTexture(tCanvasTexture);
		sCanvasSprite.setScale(CANVAS_SCALE_FACTOR, CANVAS_SCALE_FACTOR);

		// ---- DRAW BEGINS ----
		// Simulation always draws first
		wWindow.draw(sCanvasSprite);

		// Cursor Bounds
		// Lower Line
		{
			sf::Vector2i mousePos = sf::Mouse::getPosition(wWindow);
			sf::Vertex vLine[2];
			vLine[0].position = sf::Vector2f(mousePos.x - Painting::iBrushSize, mousePos.y + Painting::iBrushSize);
			vLine[0].color = sf::Color::White;
			vLine[1].position = sf::Vector2f(mousePos.x + Painting::iBrushSize, mousePos.y + Painting::iBrushSize);
			vLine[1].color = sf::Color::White;
			wWindow.draw(vLine, 2, sf::Lines);
		}
		// Upper Line
		{
			sf::Vector2i mousePos = sf::Mouse::getPosition(wWindow);
			sf::Vertex vLine[2];
			vLine[0].position = sf::Vector2f(mousePos.x - Painting::iBrushSize, mousePos.y - Painting::iBrushSize);
			vLine[0].color = sf::Color::White;
			vLine[1].position = sf::Vector2f(mousePos.x + Painting::iBrushSize, mousePos.y - Painting::iBrushSize);
			vLine[1].color = sf::Color::White;
			wWindow.draw(vLine, 2, sf::Lines);
		}
		// Left Line
		{
			sf::Vector2i mousePos = sf::Mouse::getPosition(wWindow);
			sf::Vertex vLine[2];
			vLine[0].position = sf::Vector2f(mousePos.x - Painting::iBrushSize, mousePos.y - Painting::iBrushSize);
			vLine[0].color = sf::Color::White;
			vLine[1].position = sf::Vector2f(mousePos.x - Painting::iBrushSize, mousePos.y + Painting::iBrushSize);
			vLine[1].color = sf::Color::White;
			wWindow.draw(vLine, 2, sf::Lines);
		}
		// Right Line
		{
			sf::Vector2i mousePos = sf::Mouse::getPosition(wWindow);
			sf::Vertex vLine[2];
			vLine[0].position = sf::Vector2f(mousePos.x + Painting::iBrushSize, mousePos.y - Painting::iBrushSize);
			vLine[0].color = sf::Color::White;
			vLine[1].position = sf::Vector2f(mousePos.x + Painting::iBrushSize, mousePos.y + Painting::iBrushSize);
			vLine[1].color = sf::Color::White;
			wWindow.draw(vLine, 2, sf::Lines);
		}

		// UI 
		for (int i = 0; i < static_cast<int>(TOOLBAR_BUTTONS::COUNT); ++i)
		{
			wWindow.draw(*ToolBar[i]);
		}

		// Debug metrics
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
		// Chunk lines
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
		// ---- DRAW ENDS ----
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
				{
					// First, check our UI buttons
					bool bClicked = false;
					for (int i = 0; i < static_cast<int>(TOOLBAR_BUTTONS::COUNT); ++i)
					{
						if (ToolBar[i]->OnClick([&i]()
							{
								switch (static_cast<TOOLBAR_BUTTONS>(i))
								{
									// Input tools
								case TOOLBAR_BUTTONS::PAINT:
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::IGNITE:
									Painting::bIgniting = true;
									break;
									// Elements
								case TOOLBAR_BUTTONS::WOOD:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::WOOD;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::STONE:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::ROCK;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::METAL:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::METAL;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::SAND:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::SAND;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::COAL:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::COAL;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::LEAVES:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::LEAVES;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::STEAM:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::STEAM;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::SMOKE:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::SMOKE;
									Painting::bIgniting = false;
									break;
								case TOOLBAR_BUTTONS::WATER:
									Painting::pCurrentlyPaintingParticle = PARTICLE_TYPE::WATER;
									Painting::bIgniting = false;
									break;

								default:
									break;
								}
							}))
						{
							bClicked = true;
							break;
						}
					}
					if (bClicked) { break; }


					// If we didn't click anything, run paint events
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
				}
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


						case sf::Keyboard::F5:
							SimulationSerializer::QInstance().SaveSimulation();
							break;
						case sf::Keyboard::F6:
							if (!SimulationSerializer::QInstance().LoadSimulation()) { std::cout << "Failed load\n"; }
							break;

						case sf::Keyboard::F9:
							Painting::iBrushSize = 1;
							std::cout << "Brush size: 1\n";
							break;
						case sf::Keyboard::F10:
							Painting::iBrushSize = 3;
							std::cout << "Brush size: 3\n";
							break;
						case sf::Keyboard::F11:
							Painting::iBrushSize = 5;
							std::cout << "Brush size: 5\n";
							break;
						case sf::Keyboard::F12:
							Painting::iBrushSize = 7;
							std::cout << "Brush size: 7\n";
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
					for (int x = mousePos.x - (Painting::iBrushSize / 2); x < mousePos.x + (Painting::iBrushSize / 2); ++x)
					{
						for (int y = mousePos.y - (Painting::iBrushSize / 2); y < mousePos.y + (Painting::iBrushSize / 2); ++y)
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

		// Perf capture
		--iTicksUntilPerfCapture;
		if (iTicksUntilPerfCapture <= 0)
		{
			iTicksUntilPerfCapture = iTicksPerPerfCapture;
			PerformanceReporter::QInstance().RegisterData(PerfDatum(ifps, deltaTicks, iParticleCount, iactiveParticles, iparticleVisitsTotal, ichunkVisits));
		}
	}
	PerformanceReporter::QInstance().DumpData();
}


