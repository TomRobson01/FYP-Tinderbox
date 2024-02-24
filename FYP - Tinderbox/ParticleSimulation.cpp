#include "ParticleSimulation.h"

#include "ParticleGas.h"
#include "ParticleLiquid.h"
#include "ParticlePowder.h"
#include "ParticleSolid.h"

#include <cmath>
#include <iostream>
#include <vector>

#define CREATE_PARTICLE_PTR(T, PT, PP) \
	std::make_shared<T>(iUniqueParticleID, aiX, aiY, PP)

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
#define COLOR_STEAM		sf::Color(210,	211,	212,	255)
#define COLOR_SMOKE		sf::Color(62,	65,		66,		255)
#define COLOR_FIRE		RANDOM_BOOL ? sf::Color(227, 102, 7, 255) : sf::Color(227, 157, 7, 255)
#define COLOR_CHUNK		sf::Color(53,	58,		79,		255)

#define IS_SOLID_CHECK(TYPE) \
	TYPE > PARTICLE_TYPE::SOLID && TYPE < PARTICLE_TYPE::GAS
#define IS_POWDER_CHECK(TYPE) \
	TYPE > PARTICLE_TYPE::POWDER && TYPE < PARTICLE_TYPE::SOLID
#define IS_LIQUID_CHECK(TYPE) \
	TYPE > PARTICLE_TYPE::LIQUID && TYPE < PARTICLE_TYPE::COUNT
#define IS_GAS_CHECK(TYPE) \
	TYPE > PARTICLE_TYPE::GAS && TYPE < PARTICLE_TYPE::LIQUID

std::unordered_map<PARTICLE_TYPE, SolidProperties>	solidPropertiesMap 
{
	//										Ignition Temp | Fuel Consumption | Fuel	  | Colour
	{PARTICLE_TYPE::ROCK,	SolidProperties(3000,				1,				800,	COLOR_ROCK)},
	{PARTICLE_TYPE::METAL,	SolidProperties(1000,				1,				1400,	COLOR_METAL)},
	{PARTICLE_TYPE::WOOD,	SolidProperties(100,				1,				50,		COLOR_WOOD)}
};
std::unordered_map<PARTICLE_TYPE, PowderProperties> powderPropertiesMap
{
	//											Ticks to Rest | Ignition Temp | Fuel Consumption | Fuel | Horizontal Velocity | Vertical Veloctiy	| Colour
	{PARTICLE_TYPE::SAND,		PowderProperties(1000,			100,			1,					100,		1,					1,					COLOR_SAND)},
	{PARTICLE_TYPE::COAL,		PowderProperties(1000,			1000,			0,					10000,		1,					1,					COLOR_COAL)},
	{PARTICLE_TYPE::LEAVES,		PowderProperties(1000,			5,				1,					10,			1,					1,					COLOR_LEAVES)}
};
std::unordered_map<PARTICLE_TYPE, LiquidProperties> liquidPropertiesMap
{
	//										Ticks to Rest | Extinguish Particle Type | Horizontal Velocity | Vertical Veloctiy | Colour
	{PARTICLE_TYPE::WATER,	LiquidProperties(1000,			0,							2,						4,				COLOR_WATER)}
};
std::unordered_map<PARTICLE_TYPE, GasProperties>	gasPropertiesMap
{
	//									Lifetime | Colour
	{PARTICLE_TYPE::STEAM,	GasProperties(100,		COLOR_STEAM)},
	{PARTICLE_TYPE::SMOKE,	GasProperties(100,		COLOR_SMOKE)}
};

bool bSleepingChunks[chunkCount];
bool bChunksNeedUpdating[chunkCount] = { false };
std::unordered_map<int, std::shared_ptr<Particle>> chunkParticleMaps[chunkCount];

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
void ParticleSimulation::Tick(sf::Image& arCanvas)
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

	// Pre chunk tick - cache all particles we want a given chunk index to handle
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			// First, find the chunk this particle belongs to
			const int iParticleChunkID = GetChunkForPosition(mapping.second->QX());
			if (bChunksNeedUpdating[iParticleChunkID])
			{
				mapping.second->ForceWake();
				//bChunksNeedUpdating[iParticleChunkID] = false;
			}
			if (!mapping.second->QResting() && !mapping.second->QHasLifetimeExpired())
			{

				// From there, we can construct the mapping, and 
				chunkParticleMaps[iParticleChunkID].emplace(mapping.second->QID(), mapping.second);
			}
			else
			{
				// If a particle isn't going to be drawn by the chunk tick, draw it and check if it's expired now
				if (mapping.second->QHasLifetimeExpired())
				{
					expiredParticleIDs.push_back(mapping.first);
				}
				else
				{
					const int x = mapping.second->QX();
					const int y = mapping.second->QY();
					sf::Color cCol = mapping.second->QIsOnFire() ? COLOR_FIRE : mapping.second->QColor();
					if (IsParticleOnEdge(x, y))
					{
						cCol.a = 200;
					}
					arCanvas.setPixel(x, y, cCol);
				}
			}
		}
		++iPixelsVisitted_Total;	// Pre-chunk pixel visits
		++iPixelsVisitted_PreChunk;
	}
	
	// If we destroyed ANY particle, we need to notify the chunks adjacent and including the one where the deletion took place that they need to update
	for (int i = 0; i < chunkCount; ++i)
	{
		bChunksNeedUpdating[i] = false;
	}

	// Tick each chunk
	for (int i = 0; i < chunkCount; ++i)
	{
		TickChunk(chunkParticleMaps[i], arCanvas, expiredParticleIDs);
		++iChunksVisitted;
	}

	// Clear chunk smart pointer cache, releasing their refs
	for (int i = 0; i < chunkCount; ++i)
	{
		for (std::pair<const int, std::shared_ptr<Particle>> mapping : chunkParticleMaps[i])
		{
			mapping.second.reset();
		}
		chunkParticleMaps[i].clear();
	}

	// After a tick, itterate over the particle map, and allow them to be updated again
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		mapping.second->SetHasBeenUpdated(false);
		++iPixelsVisitted_Total;	// Wake particle map visits
		++iPixelsVisitted_AllowUpdate;
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

			particleIDMap[x][y] = NULL_PARTICLE_ID;

			// Remove reference from the main and chunk hashmaps
			particleMap.erase(aiExpiredID);

			if (uiDeathParticleType != PARTICLE_TYPE::NONE)
			{
				SpawnParticle(x, y, uiDeathParticleType);
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
}

/// <summary>
/// Itterates over a single chunked area of the simulation
/// </summary>
/// <param name="auiChunkID">Index of the chunk to itterate over</param>
/// <param name="arCanvas">Canvas to draw to</param>
/// <param name="arExpiredIDs">Expired IDs vector - used to clean up expired particles at the end of the wider simulation tick</param>
/// <remarks>Note: this is not currently considered thread safe. If these were to be turned into threads as-is, we'd have each thread accessing the particleIDMap, and the hashmap, all the time.</remarks>
void ParticleSimulation::TickChunk(std::unordered_map<int, std::shared_ptr<Particle>> amParticleMap, sf::Image& arCanvas, std::vector<int>& arExpiredIDs)
{
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : amParticleMap)
	{
		if (mapping.second)
		{
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
			arCanvas.setPixel(x, y, cCol); 
			
			if (mapping.second->QHasLifetimeExpired())
			{
				arExpiredIDs.push_back(mapping.first);
				mapping.second.reset();
			}
		}
		++iPixelsVisitted_Total; // Chunk tick pixel visits
		++iPixelsVisitted_ChunkTick;
	}
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
		if (GetParticleFromMap(aiRequesterID))	// TO-DO: Right now, we're just swapping the particles, irrespective of anything blocking the way. We need to do a range check to allow for multi-pixel movements with higher velocities
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
