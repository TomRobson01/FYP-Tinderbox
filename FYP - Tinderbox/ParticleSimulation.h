#pragma once

#include <memory>
#include <unordered_map>

#include "Particle.h"

#define NULL_PARTICLE_ID 0

constexpr int simulationResolution = 256;
constexpr int chunkCount = 8;
constexpr int chunkStep = simulationResolution / chunkCount;


enum class PARTICLE_TYPE : uint8_t
{
	NONE,
	POWDER,
	SAND,
	COAL,
	LEAVES,
	SOLID,
	WOOD,
	METAL,
	ROCK,
	GAS,
	STEAM,
	SMOKE,
	LIQUID,
	WATER,
	COUNT
};

class DebugToggles
{
public:
	static DebugToggles& const QInstance()
	{
		static DebugToggles instance;
		return instance;
	};

	bool bShowPerformanceStats = false;
	bool bShowChunkBoundaries = false;
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
	bool IsSpaceOccupied(unsigned int aiX, unsigned int aiY);

	bool LineTest(int aiRequesterID, int aiStartX, int aiStartY, int aiEndX, int aiEndY, int& aiHitPointX, int& aiHitPointY);

	void ResetSimulation();

	int QParticleCount()		{ return particleMap.size(); };
	int QActiveParticleCount();
	int QParticleVisitsTotal() { return iPixelsVisitted_Total; }
	int QParticleVisitsPreChunk() { return iPixelsVisitted_PreChunk; }
	int QParticleVisitsAllowUpdate() { return iPixelsVisitted_AllowUpdate; }
	int QParticleVisitsExpiredCleanup() { return iPixelsVisitted_ExpiredCleanup; }
	int QParticleVisitsChunkTick() { return iPixelsVisitted_ChunkTick; }
	int QParticleVisitsWakeChunk() { return iPixelsVisitted_WakeChunk; }
	int QChunkVisits() { return iChunksVisitted; }
	int QBurningParticles() { return iBurningParticles; }

protected:
	void TickChunk(std::unordered_map<int, std::shared_ptr<Particle>>* amParticleMap, sf::Image* arCanvas, std::vector<int>* arExpiredIDs);

	bool IsParticleOnEdge(unsigned int aiX, unsigned int aiY);
	bool IsPointWithinSimulation(unsigned int aiX, unsigned int aiY);
	bool IsParticleDisplacementAllowed(int aiMovingParticle, int aiTargetParticle);
	std::shared_ptr<Particle> GetParticleFromMap(int aiID);

	inline int GetChunkForPosition(const int aiX);

private:
	int particleIDMap[simulationResolution][simulationResolution];
	int updatedParticleIDs[simulationResolution][simulationResolution];
	int particleHeatMap[simulationResolution][simulationResolution];

	std::unordered_map<int, std::shared_ptr<Particle>> particleMap;

	std::vector<int> forceWokenParticles;

	int iUniqueParticleID = 1; 

	int iPixelsVisitted_Total = 0;
	int iPixelsVisitted_PreChunk = 0;
	int iPixelsVisitted_WakeChunk = 0;
	int iPixelsVisitted_AllowUpdate = 0;
	int iPixelsVisitted_ExpiredCleanup = 0;
	int iPixelsVisitted_ChunkTick = 0;


	int iChunksVisitted = 0;
	int iBurningParticles = 0;
};

