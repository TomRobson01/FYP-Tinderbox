#pragma once

#include <SFML/Graphics.hpp>

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
	virtual bool QHasLifetimeExpired() { return bExpired; }

	// Core
	void		SetHasBeenUpdated(bool abNewVal)	{ bHasBeenUpdatedThisTick = abNewVal; }
	void		ForceExpire()						{ bExpired = true; }
	void		ForceWake()							{ bResting = false; }
	sf::Color	QColor()							{ return cColor; }
	int			QX()								{ return x; }
	int			QY()								{ return y; }
	bool		QHasBeenUpdatedThisTick()			{ return bHasBeenUpdatedThisTick; }
	int			QID()								{ return iParticleID; }
	bool		QResting()							{ return bResting; }

protected:
	int iParticleID;
	bool bExpired = false;
	bool bResting = false;
	bool bHasBeenUpdatedThisTick = false;
	unsigned int x, y;
	sf::Color cColor;
};

