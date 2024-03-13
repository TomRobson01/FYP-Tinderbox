#include "SimulationSerializer.h"

#include <codecvt> 
#include <locale> 
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <vector>

#define SNAPSHOT_DATUM_SIZE 4

/// <summary>
/// Create a cache of the simulation, and save it to an external file. File is CSV formatted.
/// </summary>
void SimulationSerializer::SaveSimulation()
{
	CacheSimulation();
	if (cachedSimulation.cachedParticles.size() > 0)
	{
		std::ofstream simulationFile;
		simulationFile.open("Simulation.txt");
		for (ParticleSnapshot snap : cachedSimulation.cachedParticles)
		{
			std::string sLine = std::to_string(static_cast<int>(snap.tType)) + "," + std::to_string(snap.x) + "," + std::to_string(snap.y) + "," + std::to_string(snap.iTemp) + "\n";
			simulationFile << sLine;
		}
		simulationFile.close();
	}
}

/// <summary>
/// Open the file browser and prompt the user to pick a simulation file. Parse that into a SimulationSnapshot and pass it to ApplySimulation()
/// </summary>
bool SimulationSerializer::LoadSimulation()
{
	bool bRetVal = false;
    // Attempt to open a simulation snapshot
    std::ifstream snapshotFile(SelectFile());
    if (snapshotFile.good())
    {
        // We were able to open the snaphot, parse each line
        // Note: Particle snapshots are formatted like CSVs

        SimulationSnapshot simSnap = SimulationSnapshot();
		std::string currentLine;
		while (getline(snapshotFile, currentLine))
		{
			if (currentLine.size() > 0)
			{
				std::vector<unsigned int> parsedStats;
				// Itterate over the string, reading each character into a temp string. When a comma is hit, cast that working string to an int and add to parsed stats
				std::string sWorker = "";
				for (unsigned int i = 0; i < currentLine.size(); ++i)
				{
					// If we get anything other than a number, or a comma, we're loading something we shouldn't... Return out!
					if (currentLine[i] < 48 || currentLine[i] > 57)
					{
						if (currentLine[i] != ',')
						{
							snapshotFile.close();
							return bRetVal;
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
						if (i == currentLine.size() - 1)
						{
							parsedStats.push_back(std::stoi(sWorker));
							sWorker = "";
						}
					}
				}

				if (parsedStats.size() >= SNAPSHOT_DATUM_SIZE)
				{
					// Splitting this out rather than just using the constructor for clarity
					ParticleSnapshot datum = ParticleSnapshot();
					datum.tType = static_cast<PARTICLE_TYPE>(parsedStats[0]);
					datum.x = parsedStats[1];
					datum.y = parsedStats[2];
					datum.iTemp = parsedStats[3];
					simSnap.cachedParticles.push_back(datum);
				}
			}
		}

		cachedSimulation = simSnap;
		ApplySimulation();
		bRetVal = true;
    }
    snapshotFile.close();
	return bRetVal;
}

std::string SimulationSerializer::SelectFile()
{
    std::string retVal = "";
    OPENFILENAME ofn;
    TCHAR szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // If the user selected a file, convert from lpstr to a std::string, and pass to "LoadData"
    if (GetOpenFileName(&ofn) == TRUE)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
        retVal = converter.to_bytes(std::wstring(ofn.lpstrFile));
    }

    return retVal;
}

/// <summary>
/// Caches the result of ParticleSimulation::CreateSimulationSnapshot()
/// </summary>
void SimulationSerializer::CacheSimulation()
{
	cachedSimulation = ParticleSimulation::QInstance().CreateSimulationSnapshot();
}

/// <summary>
/// Resets the particle simulation with the data from cachedSimulation
/// </summary>
void SimulationSerializer::ApplySimulation()
{
	ParticleSimulation::QInstance().ResetSimulation(cachedSimulation);
}
