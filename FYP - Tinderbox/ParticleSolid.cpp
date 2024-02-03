#include "ParticleSolid.h"

void ParticleSolid::HandleMovement()
{
	bResting = true;
}

void ParticleSolid::HandleFireProperties()
{
}

bool ParticleSolid::QHasLifetimeExpired()
{
	return bExpired;
}
