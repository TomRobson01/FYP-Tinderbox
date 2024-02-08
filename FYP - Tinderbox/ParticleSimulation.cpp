#include "ParticleSimulation.h"

#include "ParticleGas.h"
#include "ParticleLiquid.h"
#include "ParticlePowder.h"
#include "ParticleSolid.h"

#include <cmath>
#include <iostream>
#include <vector>

#define CREATE_PARTICLE_PTR(T, PT) \
	std::make_shared<T>(iUniqueParticleID, aiX, aiY, ParticleUtilities::GetColorForParticleType(PT))

#define COLOR_GREY sf::Color(150, 150, 150, 255)
#define COLOR_SAND sf::Color(237, 225, 142, 255)
#define COLOR_PINK sf::Color(197, 61, 227, 255)
#define COLOR_WATER sf::Color(90, 200, 227, 255)
#define COLOR_SMOKE sf::Color(234, 234, 255, 255)
#define COLOR_FIRE sf::Color(252, 127, 3, 255)

/// <summary>
/// Handles the updating and drawing of particles.
/// </summary>
/// <param name="arCanvas">Reference to the sf::Image to draw the simulation onto.</param>
void ParticleSimulation::Tick(sf::Image& arCanvas)
{
	std::vector<int> expiredParticleIDs;

	// Reset the particle heatmap
	//memset(particleHeatMap, 0, sizeof(particleHeatMap));

	// Itterate over every particle in the simulation and update accoringly
	for (std::pair<int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		if (mapping.second)
		{
			const int x = mapping.second->QX();
			const int y = mapping.second->QY();
			particleHeatMap[x][y] = mapping.second->QTemperature();

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
				auto HeatSurroundingsFunctor = [this](int aiX, int aiY, int aiTempStep)
					{
						if (IsPointWithinSimulation(aiX, aiY))
						{
							std::shared_ptr<Particle> pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]);
							if (pParticle)
							{
								pParticle->IncreaseTemperature(aiTempStep);
							}
						}
					};

				const int iIgnitionStep = mapping.second->QTemperature() * 0.05f;	// TO-DO: Replace this with a value in the particle itself
				HeatSurroundingsFunctor(x + 1,	y,		iIgnitionStep);
				HeatSurroundingsFunctor(x - 1,	y,		iIgnitionStep);
				HeatSurroundingsFunctor(x,		y + 1,	iIgnitionStep);
				HeatSurroundingsFunctor(x,		y - 1,	iIgnitionStep);
			}

			if (mapping.second->QHasLifetimeExpired())
			{
				expiredParticleIDs.push_back(mapping.first);
			}
			arCanvas.setPixel(x, y, mapping.second->QIsOnFire() ? COLOR_FIRE : mapping.second->QColor());	// TO-DO: Move the change in colour based on fire state into Particle::QColor()
		}
	}

	// After a tick, itterate over the particle map, and allow them to be updated again
	for (std::pair<const int, std::shared_ptr<Particle>> mapping : particleMap)
	{
		mapping.second->SetHasBeenUpdated(false);
	}

	// During the course of a tick, we check if a particle has expired it's lifetime. These particles are collected in expiredParticleIDs.
	// As our particle unordered_map stores particle objects as a shared pointer, all we need to do is erase their mapping from the
	// map, and the memory is automatically freed.
	for (int aiExpiredID : expiredParticleIDs)
	{
		const int x = GetParticleFromMap(aiExpiredID)->QX();
		const int y = GetParticleFromMap(aiExpiredID)->QY();
		particleIDMap[x][y] = NULL_PARTICLE_ID;

		particleMap.erase(aiExpiredID);

		// After expiring a particle, we need to let all of it's neighbors know that they now need to reassess their resting state
		forceWokenParticles.clear();
	}

	// If we destroyed ANY particle, we need to notify the entire simulation that they may need to update their positions
	if (expiredParticleIDs.size() > 0)
	{
		for (std::pair<const int, std::shared_ptr<Particle>> mapping : particleMap)
		{
			mapping.second->ForceWake();
		}
	}
	expiredParticleIDs.clear();
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
		if (GetParticleFromMap(aiRequesterID) && !GetParticleFromMap(particleIDMap[aiNewX][aiNewY]))	// TO-DO: Right now, we're just swapping the particles, irrespective of anything blocking the way. We need to do a range check to allow for multi-pixel movements with higher velocities
		{
			int x = GetParticleFromMap(aiRequesterID)->QX();
			int y = GetParticleFromMap(aiRequesterID)->QY();
			particleIDMap[aiNewX][aiNewY] = aiRequesterID;
			particleIDMap[x][y] = NULL_PARTICLE_ID;
			bRequestAllowed = true;
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
		std::shared_ptr<Particle> pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]);

		if (!pParticle && particleIDMap[aiX][aiY] == NULL_PARTICLE_ID)
		{
			switch (aeParticleType)
			{
			case PARTICLE_TYPE::GAS:
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticleGas, aeParticleType)));
				break;
			case PARTICLE_TYPE::LIQUID:
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticleLiquid, aeParticleType)));
				break;
			case PARTICLE_TYPE::POWDER:
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticlePowder, aeParticleType)));
				break;
			case PARTICLE_TYPE::SOLID:
				particleMap.insert(std::make_pair(iUniqueParticleID, CREATE_PARTICLE_PTR(ParticleSolid, aeParticleType)));
				break;
			default:
				return;
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
	if (IsPointWithinSimulation(aiX, aiY))
	{
		std::shared_ptr<Particle> pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]);
		if (pParticle)
		{
			pParticle->ForceExpire();
		}
	}
}

void ParticleSimulation::IgniteParticle(unsigned int aiX, unsigned int aiY)
{
	if (IsPointWithinSimulation(aiX, aiY))
	{
		std::shared_ptr<Particle> pParticle = GetParticleFromMap(particleIDMap[aiX][aiY]);
		if (pParticle && pParticle->QIgnitionTemperature() > -1)
		{
			pParticle->Ignite();
		}
	}
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
/// Utility function to grab the color associated with a particle type
/// </summary>
sf::Color const ParticleUtilities::GetColorForParticleType(PARTICLE_TYPE aeParticleType)
{
	switch (aeParticleType)
	{
	case PARTICLE_TYPE::GAS:
		return COLOR_SMOKE;
	case PARTICLE_TYPE::LIQUID:
		return COLOR_WATER;
	case PARTICLE_TYPE::POWDER:
		return COLOR_SAND;
	case PARTICLE_TYPE::SOLID:
		return COLOR_GREY;
	}
	return COLOR_PINK;
}
