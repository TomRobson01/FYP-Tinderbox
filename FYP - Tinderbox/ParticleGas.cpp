#include "ParticleGas.h"
#include "ParticleSimulation.h"

void ParticleGas::HandleMovement()
{
	// First, attempt to move upwards
	int itargetX = x;
	int itargetY = y - 1;	// TO-DO: Replace 1 with a "velocity" value

	if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
	{
		// If that failed, attempt to move horizontally one way
		itargetY = y;
		itargetX += 1;
		if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
		{
			// If that fails, then try the other way
			itargetX = x - 1;
			if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
			{
				// If all that fails, just stop
				bResting = true;
				return;
			}
		}
	}
	// Assuming we didn't return out after trying each option, assign our new internal position values to our targets
	x = itargetX;
	y = itargetY;
}

void ParticleGas::HandleFireProperties()
{
	--pProperties.iLifeTime;
}

bool ParticleGas::QHasLifetimeExpired()
{
	return pProperties.iLifeTime <= 0 || bExpired;
}