#pragma once

#include <vector>

struct PerfDatum
{
	PerfDatum() = default;
	PerfDatum(int aiFPS, int aiFrameTime, int aiParticleCount, int aiActiveParticleCount, int aiPixelVisits, int aiChunkVisits)
	{
		FPS = aiFPS;
		FrameTime = aiFrameTime;
		ParticleCount = aiParticleCount;
		ActiveParticleCount = aiActiveParticleCount;
		PixelVisits = aiPixelVisits;
		ChunkVisits = aiChunkVisits;
	}

	int FPS;
	int FrameTime;
	int ParticleCount;
	int ActiveParticleCount;
	int PixelVisits;
	int ChunkVisits;
};

class PerformanceReporter
{
public:
	static PerformanceReporter& const QInstance()
	{
		static PerformanceReporter instance;
		return instance;
	};

	void DumpData();
	void RegisterData(PerfDatum asDatum) { data.push_back(asDatum); }

private:
	std::vector<PerfDatum> data;
};

