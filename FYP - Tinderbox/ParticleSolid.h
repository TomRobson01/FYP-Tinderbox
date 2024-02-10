#pragma once
#include "Particle.h"

class ParticleSolid : public Particle
{
public:
	ParticleSolid(int aiID, unsigned int aiX, unsigned int aiY, sf::Color acColor)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		cColor = acColor;

		bResting = true;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	void Ignite() override;
	bool QHasLifetimeExpired() override;
	int QIgnitionTemperature() override;
	int QFuel() override;

private:
	int iIgnitionTemperature = 100;
	int iBurningFuelConsumption = 1;
	int iFuel = 1000;
};
