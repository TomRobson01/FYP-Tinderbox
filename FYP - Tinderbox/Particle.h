#pragma once

#include <SFML/Graphics.hpp>

enum class PARTICLE_FIRE_STATE
{
	NONE,
	BURNING,
	EXTINGUISHED
};

class Particle
{
public:
	Particle() = default;
	Particle(int aiID, unsigned int aiX, unsigned int aiY, sf::Color acColor)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		cColor = acColor;
	}

	// Overrides
	virtual void HandleMovement() {}
	virtual void HandleFireProperties() {}
	virtual void Ignite() {}
	virtual bool QHasLifetimeExpired() { return bExpired; }
	virtual int	 QIgnitionTemperature() { return -1; }
	virtual int	 QFuel() { return -1; }

	// Core
	void		SetHasBeenUpdated(bool abNewVal)	{ bHasBeenUpdatedThisTick = abNewVal; }
	void		IncreaseTemperature(int aiStep)		{ temperature += aiStep; }
	void		ForceExpire()						{ bExpired = true; }
	void		ForceWake()							{ bResting = false; }
	sf::Color	QColor()							{ return cColor; }
	int			QX()								{ return x; }
	int			QY()								{ return y; }
	bool		QHasBeenUpdatedThisTick()			{ return bHasBeenUpdatedThisTick; }
	int			QID()								{ return iParticleID; }
	bool		QResting()							{ return bResting; }
	int			QTemperature()						{ return temperature; }
	bool		QIsOnFire()							{ return eFireState == PARTICLE_FIRE_STATE::BURNING; }

protected:
	int iParticleID;
	bool bExpired = false;
	bool bResting = false;
	bool bHasBeenUpdatedThisTick = false;
	unsigned int x, y;
	sf::Color cColor;
	unsigned int temperature = 0;
	PARTICLE_FIRE_STATE eFireState;
};

