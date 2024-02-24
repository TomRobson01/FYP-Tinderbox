#pragma once

#include <SFML/Graphics.hpp>

#define FIRE_TEMP 100

enum class PARTICLE_FIRE_STATE
{
	NONE,
	BURNING,
	EXTINGUISHED
};

struct ParticleProperties
{};

class Particle
{
public:
	Particle() = default;
	Particle(int aiID, unsigned int aiX, unsigned int aiY)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
	}

	// Overrides
	virtual void	SetProperties(ParticleProperties apProperties) {}
	virtual void	HandleMovement() {}
	virtual void	HandleFireProperties() {}
	virtual void	Ignite() {}
	virtual void	ForceWake() { bResting = false; }
	virtual bool	QHasLifetimeExpired() { return bExpired; }
	virtual int		QIgnitionTemperature() { return -1; }
	virtual int		QFuel() { return -1; }
	virtual uint8_t QDeathParticleType() { return 0; }

	// Core
	void		Extinguish()						{ eFireState = PARTICLE_FIRE_STATE::NONE; temperature *= 0.5f; }
	void		SetHasBeenUpdated(bool abNewVal)	{ bHasBeenUpdatedThisTick = abNewVal; }
	void		IncreaseTemperature(int aiStep)		{ temperature += aiStep; }
	void		ForceExpire()						{ bExpired = true; }
	sf::Color	QColor()							{ return cColor; }
	int			QX()								{ return x; }
	int			QY()								{ return y; }
	bool		QHasBeenUpdatedThisTick()			{ return bHasBeenUpdatedThisTick; }
	int			QID()								{ return iParticleID; }
	bool		QResting()							{ return bResting && !QIsOnFire(); }
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
	PARTICLE_FIRE_STATE eFireState = PARTICLE_FIRE_STATE::NONE;
};

