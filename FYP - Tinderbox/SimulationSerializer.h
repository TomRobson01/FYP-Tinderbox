#pragma once

#include "ParticleSimulation.h"

class Particle;

class SimulationSerializer
{
public:
	static SimulationSerializer& const QInstance()
	{
		static SimulationSerializer instance;
		return instance;
	};

	void SaveSimulation();
	bool LoadSimulation();

	std::string SelectFile();

	void CacheSimulation();
	void ApplySimulation();

private:
	SimulationSnapshot cachedSimulation;
};

