#pragma once

#include <memory>
#include <unordered_map>

#include "Particle.h"

#define NULL_PARTICLE_ID 0
constexpr int simulationResolution = 256;

enum class PARTICLE_TYPE : uint8_t
{
	NONE,
	POWDER,
	SOLID,
	GAS,
	LIQUID,
	COUNT
};

namespace ParticleUtilities
{
	sf::Color const GetColorForParticleType(PARTICLE_TYPE aeParticleType);
};

class ParticleSimulation
{
public:
	static ParticleSimulation& const QInstance()
	{
		static ParticleSimulation instance;
		return instance;
	};

	void Tick(sf::Image& arCanvas);

	bool RequestParticleMove(int aiRequesterID, unsigned int aiNewX, unsigned int aiNewY);
	void SpawnParticle(unsigned int aiX, unsigned int aiY, PARTICLE_TYPE aeParticleType);
	void DestroyParticle(unsigned int aiX, unsigned int aiY);
	void IgniteParticle(unsigned int aiX, unsigned int aiY);
	bool ExtinguishParticle(unsigned int aiX, unsigned int aiY);
	bool ExtinguishNeighboringParticles(unsigned int aiX, unsigned int aiY);

	void ResetSimulation();

	int QParticleCount()		{ return particleMap.size(); };
	int QActiveParticleCount();

protected:
	bool IsPointWithinSimulation(unsigned int aiX, unsigned int aiY);
	std::shared_ptr<Particle> GetParticleFromMap(int aiID);

private:
	int particleIDMap[simulationResolution][simulationResolution];
	int updatedParticleIDs[simulationResolution][simulationResolution];
	int particleHeatMap[simulationResolution][simulationResolution];

	std::unordered_map<int, std::shared_ptr<Particle>> particleMap;

	std::vector<int> forceWokenParticles;

	int iUniqueParticleID = 1;

};

