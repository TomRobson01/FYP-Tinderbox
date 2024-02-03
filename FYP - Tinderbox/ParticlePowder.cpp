#include "ParticlePowder.h"
#include "ParticleSimulation.h"

void ParticlePowder::HandleMovement()
{
	// First, attempt to move downwards
	int itargetX = x;
	int itargetY = y + 1;	// TO-DO: Replace 1 with a "velocity" value

	if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
	{
		// If that failed, attempt to move diagonally one way
		itargetX += 1;
		if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
		{
			// If that fails, then try the other way
			itargetX = x - 1;
			if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
			{
				// If all that fails, just stop
				++iFailedMoveAttempts;
				if (iFailedMoveAttempts >= iAttemptsBeforeRest)
				{
					bResting = true;
				}
				return;
			}
		}
	}
	// Assuming we didn't return out after trying each option, assign our new internal position values to our targets
	x = itargetX;
	y = itargetY;
	iFailedMoveAttempts = 0;
}

void ParticlePowder::HandleFireProperties()
{
}

bool ParticlePowder::QHasLifetimeExpired()
{
	return bExpired;
}
