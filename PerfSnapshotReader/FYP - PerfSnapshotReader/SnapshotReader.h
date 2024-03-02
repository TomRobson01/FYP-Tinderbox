#pragma once

#include <fstream>
#include <string>
#include <vector>

#define SNAPSHOT_SIZE 6
struct SnapshotDatum
{
	SnapshotDatum() = default;

	// NOTE: If you add a stat here, make sure you update SNAPSHOT_SIZE to reflect the new number
	int FPS;
	int FrameTime;
	int ParticleCount;
	int ActiveParticleCount;
	int PixelVisits;
	int ChunkVisits;
	// NOTE: If you add a stat here, make sure you update SNAPSHOT_SIZE to reflect the new number
};

class SnapshotReader
{
public:
	// Singleton definition
	static SnapshotReader& QInstance()
	{
		static SnapshotReader instance;
		return instance;
	};

	bool LoadSnapshot(std::string asDirectory);
	template<typename F>
	void ForeachDataEntry(F aLambda)
	{
		for (SnapshotDatum datum : data)
		{
			aLambda(datum);
		}
	}

protected:
	bool ParseData(std::ifstream& arFStream);

private:
	std::vector<SnapshotDatum> data;
};

