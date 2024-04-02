#include "ParticleSimulation.h"

#include "ParticleGas.h"
#include "ParticleLiquid.h"
#include "ParticlePowder.h"
#include "ParticleSolid.h"

#include <SFML/Graphics.hpp>

#include <ctime>
#include <cmath>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// TO-DO: Move this to a pre-processor define
#ifdef USE_THREADED_CHUNKS
#define USE_THREADED_CHUNKS
#endif

constexpr float fFixedTickRate = 60.0f;											// Number of ticks per second
constexpr float fFixedTickInterval = (1.0f / fFixedTickRate) * CLOCKS_PER_SEC;	// Time between ticks

#define CREATE_PARTICLE_PTR(T, PT, PP) \
	std::make_shared<T>(iUniqueParticleID, aiX, aiY, static_cast<uint8_t>(PT), PP)

#define RANDOM_INT(MIN, MAX) \
	rand() % (MAX - MIN + 1) + MIN

#define RANDOM_BOOL \
	RANDOM_INT(0, 100) > 50

// Primitive Colours
#define COLOR_GREY	sf::Color(150,	150,	150,	255)
#define COLOR_PINK	sf::Color(197,	61,		227,	255)

// Element colours
#define COLOR_WOOD		sf::Color(82,	56,		33,		255)
#define COLOR_METAL		sf::Color(81,	86,		89,		255)
#define COLOR_ROCK		sf::Color(128,	134,	128,	255)
#define COLOR_SAND		sf::Color(240,	237,	161,	255)
#define COLOR_COAL		sf::Color(43,	41,		40,		255)
#define COLOR_LEAVES	sf::Color(37,	59,		35,		255)
#define COLOR_WATER		sf::Color(54,	122,	156,	255)
#define COLOR_LAVA		sf::Color(227,	157,	7,		255)
#define COLOR_STEAM		sf::Color(210,	211,	212,	255)
#define COLOR_SMOKE		sf::Color(62,	65,		66,		255)
#define COLOR_FIRE		RANDOM_BOOL ? sf::Color(227, 102, 7, 255) : sf::Color(227, 157, 7, 255)
#define COLOR_CHUNK		sf::Color(53,	58,		79,		255)

#define IS_SOLID_CHECK(TYPE) \
	(TYPE > PARTICLE_TYPE::SOLID && TYPE < PARTICLE_TYPE::GAS)
#define IS_POWDER_CHECK(TYPE) \
	(TYPE > PARTICLE_TYPE::POWDER && TYPE < PARTICLE_TYPE::SOLID)
#define IS_LIQUID_CHECK(TYPE) \
	(TYPE > PARTICLE_TYPE::LIQUID && TYPE < PARTICLE_TYPE::COUNT)
#define IS_GAS_CHECK(TYPE) \
	(TYPE > PARTICLE_TYPE::GAS && TYPE < PARTICLE_TYPE::LIQUID)

std::unordered_map<PARTICLE_TYPE, SolidProperties>	solidPropertiesMap 
{
	//										Ignition Temp | Fuel Consumption | Fuel	  | Colour			| Melting Point		| Melted particle Type	
	{PARTICLE_TYPE::ROCK,	SolidProperties(3000,				1,				400,	COLOR_ROCK,			-1,				static_cast<uint8_t>(PARTICLE_TYPE::LAVA))},
	{PARTICLE_TYPE::METAL,	SolidProperties(1000,				1,				700,	COLOR_METAL,		-1,				static_cast<uint8_t>(PARTICLE_TYPE::SMOKE)) },
	{PARTICLE_TYPE::WOOD,	SolidProperties(100,				1,				50,		COLOR_WOOD,			-1,				static_cast<uint8_t>(PARTICLE_TYPE::SMOKE)) }
};
std::unordered_map<PARTICLE_TYPE, PowderProperties> powderPropertiesMap
{
	//											Ticks to Rest | Ignition Temp | Fuel Consumption | Fuel | Horizontal Velocity | Vertical Veloctiy	| Colour
	{PARTICLE_TYPE::SAND,		PowderProperties(100,			100,			1,					100,		1,					2,					COLOR_SAND)},
	{PARTICLE_TYPE::COAL,		PowderProperties(100,			1000,			0,					1000,		1,					2,					COLOR_COAL)},
	{PARTICLE_TYPE::LEAVES,		PowderProperties(100,			5,				1,					10,			1,					3,					COLOR_LEAVES)}
};
std::unordered_map<PARTICLE_TYPE, LiquidProperties> liquidPropertiesMap
{
	//										Ticks to Rest	| Extinguish Particle Type							| Should Extinguish	| Heat Surroundings		| Horizontal Velocity | Vertical Veloctiy | Colour			| Freezing Temp		| Frozen Type										| Cooling rate
	{PARTICLE_TYPE::WATER,	LiquidProperties(100,				static_cast<uint8_t>(PARTICLE_TYPE::STEAM),				true,			false,					2,						4,				COLOR_WATER,		-25,				0,													0)},
	{PARTICLE_TYPE::LAVA,	LiquidProperties(100,				static_cast<uint8_t>(PARTICLE_TYPE::STEAM),				false,			true,					2,						2,				COLOR_LAVA,			-25,				static_cast<uint8_t>(PARTICLE_TYPE::ROCK),			100)}
};
std::unordered_map<PARTICLE_TYPE, GasProperties>	gasPropertiesMap
{
	//									Lifetime | Colour
	{PARTICLE_TYPE::STEAM,	GasProperties(100,		COLOR_STEAM)},
	{PARTICLE_TYPE::SMOKE,	GasProperties(100,		COLOR_SMOKE)}
};

std::unordered_map<PARTICLE_TYPE, sf::Image*> particleTextureAtlas;

bool bSleepingChunks[chunkCount];
bool bChunksNeedUpdating[chunkCount] = { false };
std::unordered_map<int, std::shared_ptr<Particle>> chunkParticleMaps[chunkCount];

#ifdef USE_THREADED_CHUNKS
std::mutex ParticleMapLock;
std::mutex ExpiredIDLock;
std::mutex ChunkTickLock;
#endif

template <typename F>
void ForEachParticle(std::unordered_map<int, std::shared_ptr<Particle>> aParticleMap, F afFunctor)
{
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : aParticleMap)
	{
		afFunctor(mapping);
	}
}

/// <summary>
/// Handles the updating and drawing of particles.
/// </summary>
/// <param name="arCanvas">Reference to the sf::Image to draw the simulation onto.</param>
bool ParticleSimulation::Tick(sf::Image& arCanvas)
{
	std::vector<int> expiredParticleIDs;
	iPixelsVisitted_Total = 0;
	iPixelsVisitted_PreChunk = 0;
	iPixelsVisitted_AllowUpdate = 0;
	iPixelsVisitted_ExpiredCleanup = 0;
	iPixelsVisitted_ChunkTick = 0;
	iPixelsVisitted_WakeChunk = 0;
	iChunksVisitted = 0;
	iBurningParticles = 0;

	bool bRunFullTick = false;

	clock_t cDeltaClock = clock() - cClock;
	bRunFullTick = cDeltaClock > fFixedTickInterval;

	// Pre chunk tick - cache all particles we want a given chunk index to handle
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			const int x = mapping.second->QX();
			const int y = mapping.second->QY();
			bool bHasMoved = false;

			if (bRunFullTick || bForceFullUpdate)
			{
				cClock = clock();
				bForceFullUpdate = false;

				// First, find the chunk this particle belongs to
				const int iParticleChunkID = GetChunkForPosition(mapping.second->QX());
				if (bChunksNeedUpdating[iParticleChunkID])
				{
					mapping.second->ForceWake();
				}

#ifdef USE_THREADED_CHUNKS
				if (!mapping.second->QResting() && !mapping.second->QHasLifetimeExpired())
				{
					chunkParticleMaps[iParticleChunkID].emplace(mapping.second->QID(), mapping.second);
				}
#endif

				if (!mapping.second->QResting())
				{
					if (!mapping.second->QHasBeenUpdatedThisTick())
					{
						mapping.second->HandleMovement();
						mapping.second->SetHasBeenUpdated(true);

						bHasMoved = mapping.second->QX() != x || mapping.second->QY() != y;
					}
				}

				mapping.second->HandleFireProperties();

				// If the particle is on fire, we need to heat the surroundings
				if (mapping.second->QIsOnFire())
				{
					++iBurningParticles;
					// TO-DO: Not thread safe, improve safety
					auto HeatSurroundingsFunctor = [this](int aiX, int aiY, int aiTempStep)
						{
							if (IsPointWithinSimulation(aiX, aiY))
							{
								Particle* pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]).get();
								if (pParticle)
								{
									pParticle->IncreaseTemperature(aiTempStep);
								}
							}
						};

					const int iIgnitionStep = mapping.second->QTemperature() * 0.05f;	// TO-DO: Replace this with a value in the particle itself
					HeatSurroundingsFunctor(x + 1, y, iIgnitionStep);
					HeatSurroundingsFunctor(x - 1, y, iIgnitionStep);
					HeatSurroundingsFunctor(x, y + 1, iIgnitionStep);
					HeatSurroundingsFunctor(x, y - 1, iIgnitionStep);
				}

				if (!bForceFullUpdate)
				{
					const PARTICLE_TYPE eParticleType = static_cast<PARTICLE_TYPE>(mapping.second->QType());
					sf::Color cCol = (mapping.second->QIsOnFire() && !IS_LIQUID_CHECK(eParticleType)) ? COLOR_FIRE : GetParticleColor(eParticleType, x, y, !bHasMoved);
					if (IsParticleOnEdge(x, y))
					{
						cCol.a = 170;
					}
					//sf::Color cCol = mapping.second->QResting() ? sf::Color(180, 180, 180) : sf::Color(255, 255, 255);
					arCanvas.setPixel(x, y, cCol);
				}

				if (mapping.second->QHasLifetimeExpired())
				{
					expiredParticleIDs.push_back(mapping.first);
					mapping.second.reset();
				}
			}
		}
			
		++iPixelsVisitted_Total;	// Pre-chunk pixel visits
		++iPixelsVisitted_PreChunk;
	}
#ifdef USE_THREADED_CHUNKS
	// Spin up chunk update threads
	std::thread worker1([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[0], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker2([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[1], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker3([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[2], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker4([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[3], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker5([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[4], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker6([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[5], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker7([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[6], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	std::thread worker8([this, &arCanvas, &expiredParticleIDs]() { TickChunk(&chunkParticleMaps[7], &arCanvas, &expiredParticleIDs); });
	++iChunksVisitted;
	worker1.join();
	worker2.join();
	worker3.join();
	worker4.join();
	worker5.join();
	worker6.join();
	worker7.join();
	worker8.join();

	// Clear chunk smart pointer cache, releasing their refs
	for (int i = 0; i < chunkCount; ++i)
	{
		for (std::pair<const int, std::shared_ptr<Particle>> mapping : chunkParticleMaps[i])
		{
			mapping.second.reset();
		}
		chunkParticleMaps[i].clear();
	}
#endif

	// After a tick, itterate over the particle map, and allow them to be updated again
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		mapping.second->SetHasBeenUpdated(false);
		++iPixelsVisitted_Total;	// Wake particle map visits
		++iPixelsVisitted_AllowUpdate;
	}

	// Reset chunks requiring updates
	if (bRunFullTick)
	{
		for (int i = 0; i < chunkCount; ++i)
		{
			bChunksNeedUpdating[i] = false;
		}
	}

	// During the course of a tick, we check if a particle has expired it's lifetime. These particles are collected in expiredParticleIDs.
	// As our particle unordered_map stores particle objects as a shared pointer, all we need to do is erase their mapping from the
	// map, and the memory is automatically freed.
	for (int aiExpiredID : expiredParticleIDs)
	{
		if (GetParticleFromMap(aiExpiredID))
		{
			const int x = GetParticleFromMap(aiExpiredID)->QX();
			const int y = GetParticleFromMap(aiExpiredID)->QY();

			const PARTICLE_TYPE uiDeathParticleType = static_cast<PARTICLE_TYPE>(GetParticleFromMap(aiExpiredID)->QDeathParticleType());
			bool bCanSpawnDeathParticle = IS_SOLID_CHECK(static_cast<PARTICLE_TYPE>(GetParticleFromMap(aiExpiredID)->QType())) || IS_LIQUID_CHECK(static_cast<PARTICLE_TYPE>(GetParticleFromMap(aiExpiredID)->QType()));

			particleIDMap[x][y] = NULL_PARTICLE_ID;

			// Remove reference from the main and chunk hashmaps
			particleMap.erase(aiExpiredID);

			if (bCanSpawnDeathParticle)
			{
				if (uiDeathParticleType != PARTICLE_TYPE::NONE)
				{
					SpawnParticle(x, y, uiDeathParticleType);
				}
			}

			// Cache any chunks we need to notify as a result of this deletion
			const int iParticleChunkID = GetChunkForPosition(x);
			bChunksNeedUpdating[iParticleChunkID] = true;
			if (iParticleChunkID < chunkCount)
			{
				bChunksNeedUpdating[iParticleChunkID + 1] = true;
			}
			if (iParticleChunkID > 0)
			{
				bChunksNeedUpdating[iParticleChunkID - 1] = true;
			}

			++iPixelsVisitted_Total;	// Clean up expired pixel visits
			++iPixelsVisitted_ExpiredCleanup;
		}
	}
	expiredParticleIDs.clear();

	return bRunFullTick;
}

/// <summary>
/// Initializes any cached data for the simulation, such as the particle texture atlas
/// </summary>
void ParticleSimulation::Initialize()
{
	auto TextureLoaderFunctor = [&](PARTICLE_TYPE aeParticleType, std::string asFilepath)
		{
			sf::Image* img = new sf::Image();
			if (img->loadFromFile(asFilepath))
			{
				particleTextureAtlas.emplace(aeParticleType, img);
			}
		};

	// Load all particle textures into the atlas here
	TextureLoaderFunctor(PARTICLE_TYPE::COAL, "Assets\\Sprites\\T_Coal.png");
	TextureLoaderFunctor(PARTICLE_TYPE::SAND, "Assets\\Sprites\\T_Sand.png");
	TextureLoaderFunctor(PARTICLE_TYPE::LEAVES, "Assets\\Sprites\\T_Leaves.png");
	TextureLoaderFunctor(PARTICLE_TYPE::WOOD, "Assets\\Sprites\\T_Wood.png");
	TextureLoaderFunctor(PARTICLE_TYPE::ROCK, "Assets\\Sprites\\T_Stone.png");
}

/// <summary>
/// Itterates over a single chunked area of the simulation
/// </summary>
/// <param name="auiChunkID">Index of the chunk to itterate over</param>
/// <param name="arCanvas">Canvas to draw to</param>
/// <param name="arExpiredIDs">Expired IDs vector - used to clean up expired particles at the end of the wider simulation tick</param>
/// <remarks>Note: this is not currently considered thread safe. If these were to be turned into threads as-is, we'd have each thread accessing the particleIDMap, and the hashmap, all the time.</remarks>
void ParticleSimulation::TickChunk(std::unordered_map<int, std::shared_ptr<Particle>>* amParticleMap, sf::Image* arCanvas, std::vector<int>* arExpiredIDs)
{
#ifdef USE_THREADED_CHUNKS
	if (!amParticleMap || !arCanvas || !arExpiredIDs)
	{
		return;
	}

	for (std::pair<const int, std::shared_ptr<Particle>> mapping : *amParticleMap)
	{
		if (mapping.second)
		{
			ParticleMapLock.lock();
			const int x = mapping.second->QX();
			const int y = mapping.second->QY();

			if (!mapping.second->QResting())
			{
				if (!mapping.second->QHasBeenUpdatedThisTick())
				{
					mapping.second->HandleMovement();
					mapping.second->SetHasBeenUpdated(true);
				}
			}

			mapping.second->HandleFireProperties();

			// If the particle is on fire, we need to heat the surroundings
			if (mapping.second->QIsOnFire())
			{
				++iBurningParticles;
				// TO-DO: Not thread safe, improve safety
				auto HeatSurroundingsFunctor = [this](int aiX, int aiY, int aiTempStep)
					{
						if (IsPointWithinSimulation(aiX, aiY))
						{
							Particle* pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]).get();
							if (pParticle)
							{
								pParticle->IncreaseTemperature(aiTempStep);
							}
						}
					};

				const int iIgnitionStep = mapping.second->QTemperature() * 0.05f;	// TO-DO: Replace this with a value in the particle itself
				HeatSurroundingsFunctor(x + 1, y, iIgnitionStep);
				HeatSurroundingsFunctor(x - 1, y, iIgnitionStep);
				HeatSurroundingsFunctor(x, y + 1, iIgnitionStep);
				HeatSurroundingsFunctor(x, y - 1, iIgnitionStep);
			}

			sf::Color cCol = mapping.second->QIsOnFire() ? COLOR_FIRE : mapping.second->QColor();
			if (IsParticleOnEdge(x, y))
			{
				cCol.a = 200;
			}
			arCanvas->setPixel(x, y, cCol); 
			
			if (mapping.second->QHasLifetimeExpired())
			{
				ExpiredIDLock.lock();
				arExpiredIDs->push_back(mapping.first);
				ExpiredIDLock.unlock();
				mapping.second.reset();
			}
			ParticleMapLock.unlock();
		}
		++iPixelsVisitted_Total; // Chunk tick pixel visits
		++iPixelsVisitted_ChunkTick;
	}
#endif
}

/// <summary>
/// Handles the movement of particles.
/// </summary>
/// <param name="aiRequesterID">The ID of the particle to move.</param>
/// <param name="aiNewX">The X position to move the particle to.</param>
/// <param name="aiNewY">The Y position to move the particle to.</param>
/// <returns>True if the move could be completed, false otherwise.</returns>
bool ParticleSimulation::RequestParticleMove(int aiRequesterID, unsigned int aiNewX, unsigned int aiNewY)
{
	bool bRequestAllowed = false;

	if (IsPointWithinSimulation(aiNewX, aiNewY))
	{
		if (GetParticleFromMap(aiRequesterID))
		{
			// Check if the slot is free
			// If so, move the particle into a new slow
			// If not, we need to check for any special cases
			if (GetParticleFromMap(particleIDMap[aiNewX][aiNewY]))
			{
				// Special case: powders can displace water, swapping with them
				if (IsParticleDisplacementAllowed(aiRequesterID, particleIDMap[aiNewX][aiNewY]))
				{
					int x = GetParticleFromMap(aiRequesterID)->QX();
					int y = GetParticleFromMap(aiRequesterID)->QY();

					const unsigned int uiDisplacedID = particleIDMap[aiNewX][aiNewY];

					// Finally, swap the particles
					particleIDMap[aiNewX][aiNewY] = aiRequesterID;
					particleIDMap[x][y] = uiDisplacedID;
					bRequestAllowed = true;
				}
			}
			else
			{
				int x = GetParticleFromMap(aiRequesterID)->QX();
				int y = GetParticleFromMap(aiRequesterID)->QY();

				// Finally, move the particle
				particleIDMap[aiNewX][aiNewY] = aiRequesterID;
				particleIDMap[x][y] = NULL_PARTICLE_ID;
				bRequestAllowed = true;
			}
		}
	}
	return bRequestAllowed;
}

/// <summary>
/// Safely spawns a particle at a given spot in the simulation.
/// </summary>
/// <param name="aiX">The X position to spawn the new particle.</param>
/// <param name="aiY">The Y position to spawn the new particle.</param>
/// <param name="aeParticleType">The type of particle to spawn.</param>
/// <remarks>No particle will be spawned if the given position is not within the simulation; nor if that position is already taken.</remarks>
void ParticleSimulation::SpawnParticle(unsigned int aiX, unsigned int aiY, PARTICLE_TYPE aeParticleType)
{
	if (IsPointWithinSimulation(aiX, aiY))
	{
		Particle* pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]).get();

		if (!pParticle && particleIDMap[aiX][aiY] == NULL_PARTICLE_ID)
		{
			if (IS_GAS_CHECK(aeParticleType))
			{
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticleGas, aeParticleType, gasPropertiesMap.at(aeParticleType))));
			}
			if (IS_LIQUID_CHECK(aeParticleType))
			{
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticleLiquid, aeParticleType, liquidPropertiesMap.at(aeParticleType))));
			}
			if (IS_POWDER_CHECK(aeParticleType))
			{
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticlePowder, aeParticleType, powderPropertiesMap.at(aeParticleType))));
			}
			if (IS_SOLID_CHECK(aeParticleType))
			{
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticleSolid, aeParticleType, solidPropertiesMap.at(aeParticleType))));
			}

			particleIDMap[aiX][aiY] = iUniqueParticleID;
			++iUniqueParticleID;
		}
	}
}

/// <summary>
/// Marks a given point in the simulation as expired - it will be deleted when the current tick cleans up any expired particles
/// </summary>
void ParticleSimulation::DestroyParticle(unsigned int aiX, unsigned int aiY)
{
	if (IsPointWithinSimulation(aiX, aiY) && IsSpaceOccupied(aiX, aiY))
	{
		GetParticleFromMap(particleIDMap[aiX][aiY])->ForceExpire();
	}
}

/// <summary>
/// Notifys a particle in the simulation to ignite
/// </summary>
/// <param name="aiX">The X position of the target particle.</param>
/// <param name="aiY">The Y position of the target particle.</param>
void ParticleSimulation::IgniteParticle(unsigned int aiX, unsigned int aiY)
{
	if (IsPointWithinSimulation(aiX, aiY) && IsSpaceOccupied(aiX, aiY))
	{
		GetParticleFromMap(particleIDMap[aiX][aiY])->Ignite();
	}
}

/// <summary>
/// Notifys a particle in the simulation to extinguish
/// </summary>
/// <param name="aiX">The X position of the target particle.</param>
/// <param name="aiY">The Y position of the target particle.</param>
/// <returns>True if the particle was extinguished, false otherwise.</returns>
bool ParticleSimulation::ExtinguishParticle(unsigned int aiX, unsigned int aiY)
{
	bool bRetVal = false;
	if (IsPointWithinSimulation(aiX, aiY) && IsSpaceOccupied(aiX, aiY))
	{
		GetParticleFromMap(particleIDMap[aiX][aiY])->Extinguish();
	}
	return bRetVal;
}

/// <summary>
/// Notifys all particles surrounding a point to extinguish.
/// </summary>
/// <param name="aiX">The X position of the particle to extinguish around.</param>
/// <param name="aiY">The Y position of the particle to extinguish around.</param>
/// <returns>True if any particle was extinguished, false otherwise.</returns>
bool ParticleSimulation::ExtinguishNeighboringParticles(unsigned int aiX, unsigned int aiY)
{
	return ExtinguishParticle(aiX + 1, aiY) || ExtinguishParticle(aiX - 1, aiY) || ExtinguishParticle(aiX, aiY + 1) || ExtinguishParticle(aiX, aiY - 1);
}

/// <summary>
/// Helper function to check if a given space is occupied.
/// </summary>
/// <param name="aiX">The X position of the target particle.</param>
/// <param name="aiY">The Y position of the target particle.</param>
bool ParticleSimulation::IsSpaceOccupied(unsigned int aiX, unsigned int aiY)
{
	bool bRetVal = false;
	if (IsPointWithinSimulation(aiX, aiY))
	{
		if (GetParticleFromMap(particleIDMap[aiX][aiY]).get())
		{
			bRetVal = true;
		}
	}
	return bRetVal;
}

/// <summary>
/// Using a DDA algorithm, trace a line between the start and end points, checking if any of the points traversed are occupied.
/// </summary>
/// <param name="aiRequesterID">The ID of the particle that is moving.</param>
/// <param name="aiStartX">The start position of the line on the X axis.</param>
/// <param name="aiStartY">The start position of the line on the Y axis.</param>
/// <param name="aiEndX">The end position of the line on the X axis.</param>
/// <param name="aiEndY">The end position of the line on the Y axis.</param>
/// <param name="aiHitPointX">The out variable for where the particle can move without collision on the X axis.</param>
/// <param name="aiHitPointY">The out variable for where the particle can move without collision on the Y axis.</param>
/// <returns>True if the particle is able to move at all.</returns>
/// <remarks>DDA alogirthm: https://en.wikipedia.org/wiki/Digital_differential_analyzer_(graphics_algorithm) </remarks>
bool ParticleSimulation::LineTest(int aiRequesterID, int aiStartX, int aiStartY, int aiEndX, int aiEndY, int& aiHitPointX, int& aiHitPointY)
{
	bool bRetVal = true;

	float fDeltaX = (aiEndX - aiStartX);
	float fDeltaY = (aiEndY - aiStartY);

	const float fStep = abs(fDeltaX) >= abs(fDeltaY) ? abs(fDeltaX) : abs(fDeltaY);

	fDeltaX /= fStep;
	fDeltaY /= fStep;

	float fX = aiStartX;
	float fY = aiStartY;
	int i = 0;
	for (int i = 0; i <= fStep; ++i)
	{
		int x = fX;
		int y = fY;
		if (!IsPointWithinSimulation(x, y) || (particleIDMap[x][y] != aiRequesterID && GetParticleFromMap(particleIDMap[x][y])))
		{
			if (IsParticleDisplacementAllowed(aiRequesterID, particleIDMap[x][y]))
			{
				aiHitPointX = x;
				aiHitPointY = y;
			}
			break;
		}

		aiHitPointX = x;
		aiHitPointY = y;
		fX += fDeltaX;
		fY += fDeltaY;
	}

	bRetVal = aiStartX != aiHitPointX && aiStartY != aiHitPointY;

	return bRetVal;
}

/// <summary>
/// Caches the current state of each particle in particleMap as a ParticleSnapshot, returning them in a SimulationSnapshot
/// </summary>
SimulationSnapshot ParticleSimulation::CreateSimulationSnapshot()
{
	SimulationSnapshot retVal = SimulationSnapshot();


#ifdef USE_THREADED_CHUNKS
	ChunkTickLock.lock();
	ParticleMapLock.lock();
#endif
	for (std::pair<int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			ParticleSnapshot snap = ParticleSnapshot();
			snap.tType = static_cast<PARTICLE_TYPE>(mapping.second->QType());
			snap.iTemp = mapping.second->QTemperature();
			snap.x = mapping.second->QX();
			snap.y = mapping.second->QY();
			retVal.cachedParticles.push_back(snap);
		}
	}
#ifdef USE_THREADED_CHUNKS
	ParticleMapLock.unlock();
	ChunkTickLock.unlock();
#endif

	std::cout << "Snapshot taken!\n";
	return retVal;
}

/// <summary>
/// Marks all particles in the simulation as expired, ready for deletion in the next tick
/// </summary>
void ParticleSimulation::ResetSimulation()
{
	for (std::pair<int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			mapping.second->ForceExpire();
		}
	}
	bForceFullUpdate = true;
}

/// <summary>
/// Forceably deletes all current particles and spawns new ones based on a snapshot
/// </summary>
/// <remarks>Acquires ParticleMapLock to ensure security when resetting the particle map.</remarks>
void ParticleSimulation::ResetSimulation(SimulationSnapshot asSnapshot)
{
#ifdef USE_THREADED_CHUNKS
	ParticleMapLock.lock();
#endif
	// First, release all smart pointers in the existing particle map
	for (std::pair<int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			mapping.second.reset();
		}
	}
	particleMap.clear();
	for (int x = 0; x < simulationResolution; ++x)
	{

		for (int y = 0; y < simulationResolution; ++y)
		{
			particleIDMap[x][y] = NULL_PARTICLE_ID;
			particleHeatMap[x][y] = 0;
			updatedParticleIDs[x][y] = NULL_PARTICLE_ID;
		}
	}

	// Then create new particles from the particle snapshots
	for (ParticleSnapshot snap : asSnapshot.cachedParticles)
	{
		SpawnParticle(snap.x, snap.y, snap.tType);
	}

	std::cout << "Snapshot applied!\n";
#ifdef USE_THREADED_CHUNKS
	ParticleMapLock.unlock();
#endif
}

/// <summary>
/// Helper function to check the number of particles that are currently at full processing.
/// </summary>
/// <remarks>Itterates over the entire particleMap hashmap. Best to use just as a debugging function. Note: This may be stripped out of release builds at a later date.</remarks>
int ParticleSimulation::QActiveParticleCount()
{
	int iRestingParticleCount = 0;
	for (std::pair<int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			if (mapping.second->QResting())
			{
				++iRestingParticleCount;
			}
		}
	}
	return particleMap.size() - iRestingParticleCount;
}

/// <summary>
/// Helper function to check if a point is within the bounds of the simulation.
/// </summary>
bool ParticleSimulation::IsPointWithinSimulation(unsigned int aiX, unsigned int aiY)
{
	return aiX < simulationResolution&& aiY < simulationResolution&& aiX >= 0 && aiY >= 0;
}

/// <summary>
/// Helper function to determine any special cases where a particle can displace another/move into anothers space
/// </summary>
/// <param name="aiMovingParticleID">The ID for the moving particle.</param>
/// <param name="aiTargetParticleID">The ID for the displaced particle.</param>
bool ParticleSimulation::IsParticleDisplacementAllowed(int aiMovingParticleID, int aiTargetParticleID)
{
	bool bAllowDisplacement = false;
	
	// Special case: Powders can displace liquids
	bAllowDisplacement = dynamic_cast<ParticleLiquid*>(GetParticleFromMap(aiTargetParticleID).get()) && dynamic_cast<ParticlePowder*>(GetParticleFromMap(aiMovingParticleID).get());

	return bAllowDisplacement;
}

/// <summary>
/// Helper function to access a partcile from particleMap
/// </summary>
/// <param name="aiID">The ID of the particle you wish toa ccess in the unordered_map</param>
/// <returns>shared_ptr associated with the passed ID. Nullptr if no mapping exists with said ID.</returns>
std::shared_ptr<Particle> ParticleSimulation::GetParticleFromMap(int aiID)
{
	if (particleMap.find(aiID) != particleMap.end())
	{
		return particleMap.at(aiID);
	}
	return nullptr;
}

/// <summary>
/// Helper function to find the chunk a given position belongs to
/// </summary>
inline int ParticleSimulation::GetChunkForPosition(const int aiX)
{
	int iParticleChunkID = 0;
	for (int i = chunkStep; i < simulationResolution; i += chunkStep)
	{
		if (aiX >= i)
		{
			++iParticleChunkID;
			continue;
		}
		break;
	}
	return iParticleChunkID;
}

/// <summary>
/// Helper function to get the colour for a particle - either a solid colour, or sampled from a texture
/// </summary>
/// <param name="aeParticleType">The type of particle we're requesting the colour of</param>
/// <param name="aiX">The X position of the particle - used for sampling</param>
/// <param name="aiY">The T position of the particle - used for sampling</param>
/// <param name="abUseTexture">Whether or not to sample a texture, if applicable for aeParticleType</param>
sf::Color ParticleSimulation::GetParticleColor(PARTICLE_TYPE aeParticleType, unsigned int aiX, unsigned int aiY, bool abUseTexture)
{
	if (particleTextureAtlas.find(aeParticleType) != particleTextureAtlas.end() && abUseTexture)
	{
		// If the particle type has a texture loaded, sample that
		sf::Vector2u uvSize = particleTextureAtlas.at(aeParticleType)->getSize();
		sf::Vector2u sampleUV = sf::Vector2u(aiX, aiY);
		if (aiX >= uvSize.x)
		{
			sampleUV.x %= uvSize.x;
		}
		if (aiY >= uvSize.y)
		{
			sampleUV.y %= uvSize.y;
		}

		return particleTextureAtlas.at(aeParticleType)->getPixel(sampleUV.x, sampleUV.y);
	}
	else
	{
		// If not, pull from the pre-defined colours
		switch (aeParticleType)
		{
		case PARTICLE_TYPE::SAND:
			return COLOR_SAND;
		case PARTICLE_TYPE::COAL:
			return COLOR_COAL;
		case PARTICLE_TYPE::LEAVES:
			return COLOR_LEAVES;
		case PARTICLE_TYPE::WOOD:
			return COLOR_WOOD;
		case PARTICLE_TYPE::METAL:
			return COLOR_METAL;
		case PARTICLE_TYPE::ROCK:
			return COLOR_ROCK;
		case PARTICLE_TYPE::STEAM:
			return COLOR_STEAM;
		case PARTICLE_TYPE::SMOKE:
			return COLOR_SMOKE;
		case PARTICLE_TYPE::WATER:
			return COLOR_WATER;
		case PARTICLE_TYPE::LAVA:
			return COLOR_LAVA;
		}
	}
	return sf::Color(255,255,255, 0);	// Transparent
}

/// <summary>
/// Helper function to detect a particle on the edge of a shape
/// </summary>
bool ParticleSimulation::IsParticleOnEdge(unsigned int aiX, unsigned int aiY)
{
	bool bRetVal = false;

	auto NeighborCheckFunctor = [&bRetVal, this](int aiTargetX, int aiTargetY)
	{
		if (!bRetVal)
			bRetVal = IsPointWithinSimulation(aiTargetX, aiTargetY) && !IsSpaceOccupied(aiTargetX, aiTargetY);
	};

	NeighborCheckFunctor(aiX + 1,	aiY);
	NeighborCheckFunctor(aiX - 1,	aiY);
	NeighborCheckFunctor(aiX,		aiY + 1);
	NeighborCheckFunctor(aiX,		aiY - 1);

	return bRetVal;
}
