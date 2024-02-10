#include "ParticleLiquid.h"
#include "ParticleSimulation.h"

/// <summary>
/// Handles the movement logic for the particle
/// </summary>
void ParticleLiquid::HandleMovement()
{
	// First, attempt to move downwards
	int itargetX = x;
	int itargetY = y + iVelocityY;

	ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX , itargetY);
	if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
	{
		// If that failed, attempt to move horizontally one way
		itargetY = y;
		itargetX += iVelocityX;
		ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX, itargetY);
		if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
		{
			// If that fails, then try the other way
			itargetX = x - iVelocityX;
			ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX, itargetY);
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

/// <summary>
/// Handles fire propogation logic for this particle
/// </summary>
void ParticleLiquid::HandleFireProperties()
{
	if (ParticleSimulation::QInstance().ExtinguishNeighboringParticles(x, y))
	{
		uiDeathParticleType = static_cast<uint8_t>(PARTICLE_TYPE::GAS);
		bExpired = true;
	}
}

/// <summary>
/// Checks if this particle is expired, and should be removed up at the end of this tick
/// </summary>
bool ParticleLiquid::QHasLifetimeExpired()
{
	return bExpired;
}

/// <summary>
/// Returns the type of particle to spawn when this particle is removed
/// </summary>
uint8_t ParticleLiquid::QDeathParticleType()
{
	return uiDeathParticleType;
}
