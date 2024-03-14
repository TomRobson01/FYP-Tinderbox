#pragma once
#include "Particle.h"

struct SolidProperties
{
	SolidProperties() = default;
	SolidProperties(int aiIgnitionTemperature, int aiBurningFuelConsumption, int aiFuel, sf::Color acColor, int aiMeltingPoint, uint8_t auiMeltParticleType)
	{
		iIgnitionTemperature = aiIgnitionTemperature;
		iBurningFuelConsumption = aiBurningFuelConsumption;
		iFuel = aiFuel;
		cColor = acColor;
		uiMeltedParticleType = auiMeltParticleType;
		iMeltingPoint = aiMeltingPoint;
	}

	int iIgnitionTemperature = 100;
	int iMeltingPoint = -1;
	uint8_t uiMeltedParticleType = 0;
	int iBurningFuelConsumption = 1;
	int iFuel = 1000;
	sf::Color cColor;
};

class ParticleSolid : public Particle
{
public:
	ParticleSolid(int aiID, unsigned int aiX, unsigned int aiY, uint8_t auiParticleType, SolidProperties apProperties)
	{
		iParticleID = aiID;
		uiParticleType = auiParticleType;
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
	uint8_t QDeathParticleType() override;

private:
	bool bMelted = false;
	SolidProperties pProperties;
};
