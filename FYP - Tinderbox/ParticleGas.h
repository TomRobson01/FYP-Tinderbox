#pragma once
#include "Particle.h"

struct GasProperties
{
	GasProperties() = default;
	GasProperties(int aiLifeTime, sf::Color acColor)
	{
		iLifeTime = aiLifeTime;
		cColor = acColor;
	}
	int iLifeTime = 100;
	sf::Color cColor;
};

class ParticleGas : public Particle
{
public:
	ParticleGas(int aiID, unsigned int aiX, unsigned int aiY, uint8_t auiParticleType, GasProperties apProperties)
	{
		iParticleID = aiID;
		uiParticleType = auiParticleType;
		x = aiX;
		y = aiY;
		pProperties = apProperties;
		cColor = pProperties.cColor;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	bool QHasLifetimeExpired() override;

private:
	GasProperties pProperties;
};

