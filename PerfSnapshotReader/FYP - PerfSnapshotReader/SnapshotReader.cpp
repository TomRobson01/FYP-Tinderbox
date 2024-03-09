#include "SnapshotReader.h"

/// <summary>
/// Attempts to open the file at the given directory. If successful, wil also call SnapshotReader::ParseData
/// </summary>
/// <param name="asDirectory">The directory of the file to load</param>
/// <returns>True if the file could be loaded AND parsed</returns>
bool SnapshotReader::LoadSnapshot(std::string asDirectory)
{
	bool bRetVal = false;
	std::ifstream snapshotFile(asDirectory);
	if (snapshotFile.good())
	{
		bRetVal = ParseData(snapshotFile);
	}
	snapshotFile.close();
	return bRetVal;
}

/// <summary>
/// Parses a filestream and tries to store the data in the correct formats
/// </summary>
/// <param name="arFStream">The filetstream to read from</param>
/// <returns>True if the data could be successfully parsed and formatted, false if we gain insuffient data, or hit a non-numeric character (excluding a comma)</returns>
bool SnapshotReader::ParseData(std::ifstream& arFStream)
{
	bool bRetVal = false;
	data.clear();

	std::string currentLine;
	while (getline(arFStream, currentLine))
	{
		if (currentLine.size() > 0)
		{
			std::vector<int> parsedStats;
			// Itterate over the string, reading each character into a temp string. When a comma is hit, cast that working string to an int and add to parsed stats
			std::string sWorker = "";
			for (unsigned int i = 0; i < currentLine.size(); ++i) 
			{
				// If we get anything other than a number, or a comma, we're loading something we shouldn't... Return out!
				if (currentLine[i] < 48 || currentLine[i] > 57)
				{
					if (currentLine[i] != ',')
					{
						return false;
					}
				}

				if (currentLine[i] == ',')
				{
					if (sWorker.size() > 0)
					{
						parsedStats.push_back(std::stoi(sWorker));
						sWorker = "";
					}
				}
				else
				{
					sWorker += currentLine[i];
					if (currentLine[i] == currentLine[currentLine.size() - 1])
					{
						parsedStats.push_back(std::stoi(sWorker));
						sWorker = "";
					}
				}
			}

			if (parsedStats.size() >= SNAPSHOT_SIZE)
			{
				// Splitting this out rather than just using the constructor for clarity
				SnapshotDatum datum = SnapshotDatum();
				datum.FPS = parsedStats[0];
				datum.FrameTime = parsedStats[1];
				datum.ParticleCount = parsedStats[2];
				datum.ActiveParticleCount = parsedStats[3];
				datum.PixelVisits = parsedStats[4];
				datum.ChunkVisits = parsedStats[5];

				if (datum.FPS > 0)
				{
					data.push_back(datum);
				}
			}
		}
	}
	bRetVal = data.size() > 0;
	return bRetVal;
}
