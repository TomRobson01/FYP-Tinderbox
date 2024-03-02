#include "PerformanceReporter.h"

#include <fstream>

void PerformanceReporter::DumpData()
{
	if (data.size() > 0)
	{
		std::ofstream dumpFile;
		dumpFile.open("Report.txt");
		for (PerfDatum d : data)
		{
			dumpFile << d.FPS << "," << d.FrameTime << "," << d.ParticleCount << "," << d.ActiveParticleCount << "," << d.PixelVisits << "," << d.ChunkVisits << "\n";
		}
		dumpFile.close();
	}
}
