#pragma once
#include "Particle.h"

struct PowderProperties
{
	PowderProperties() = default;
	PowderProperties(int aiAttemptsBeforeRest, int aiIgnitionTemperature, int aiBurningFuelConsumption, int aiFuel, int aiVelocityX, int aiVelocityY, sf::Color acColor)
	{
		iAttemptsBeforeRest = aiAttemptsBeforeRest;
		iFailedMoveAttempts = 0;
		iIgnitionTemperature = aiIgnitionTemperature;
		iBurningFuelConsumption = aiBurningFuelConsumption;
		iFuel = aiFuel;
		iVelocityX = aiVelocityX;
		iVelocityY = aiVelocityY;
		cColor = acColor;
	}

	int iAttemptsBeforeRest = 1000;
	int iFailedMoveAttempts = 0;
	int iIgnitionTemperature = 100;
	int iBurningFuelConsumption = 1;
	int iFuel = 200;
	int iVelocityX = 1;
	int iVelocityY = 1;
	sf::Color cColor;
};

class ParticlePowder : public Particle
{
public:
	ParticlePowder(int aiID, unsigned int aiX, unsigned int aiY, PowderProperties apProperties)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		pProperties = apProperties;
		cColor = pProperties.cColor;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	void Ignite() override;
	void ForceWake() override;
	bool QHasLifetimeExpired() override;
	int QIgnitionTemperature() override;
	int QFuel() override;

private:
	PowderProperties pProperties;
};

