#pragma once
#include "Particle.h"

struct SolidProperties
{
	SolidProperties() = default;
	SolidProperties(int aiIgnitionTemperature, int aiBurningFuelConsumption, int aiFuel, sf::Color acColor)
	{
		iIgnitionTemperature = aiIgnitionTemperature;
		iBurningFuelConsumption = aiBurningFuelConsumption;
		iFuel = aiFuel;
		cColor = acColor;
	}

	int iIgnitionTemperature = 100;
	int iBurningFuelConsumption = 1;
	int iFuel = 1000;
	sf::Color cColor;
};

class ParticleSolid : public Particle
{
public:
	ParticleSolid(int aiID, unsigned int aiX, unsigned int aiY, SolidProperties apProperties)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		pProperties = apProperties;
		cColor = pProperties.cColor;

		bResting = true;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	void Ignite() override;
	bool QHasLifetimeExpired() override;
	int QIgnitionTemperature() override;
	int QFuel() override;

private:
	SolidProperties pProperties;
};
