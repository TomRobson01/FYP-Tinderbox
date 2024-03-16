#include "UIButton.h"
#include <iostream>

void UIButton::Initialize(sf::Vector2f aiPosition, std::string asBasePath, std::string asHoveredPath)
{
	setPosition(aiPosition);
	tBaseTexture.loadFromFile(asBasePath);
	tHoveredTexture.loadFromFile(asHoveredPath);

	setTexture(tBaseTexture);
}

void UIButton::Tick(sf::Vector2i aiMousePos)
{
	// First grab the distance between the mouse and this button
	/*float fDeltaX = aiMousePos.x - (getPosition().x + 0);
	float fDeltaY = aiMousePos.y - (getPosition().y + 0);
	float fSqDistance = (fDeltaX * fDeltaX) + (fDeltaY * fDeltaY);*/

	const bool bXHover = aiMousePos.x < getPosition().x + fRadius && aiMousePos.x > getPosition().x;
	const bool bYHover = aiMousePos.y < getPosition().y + fRadius && aiMousePos.y > getPosition().y;

	// If that distance is less than the radius, we're hovered
	const bool bCurrentHoverState = bHovered;
	//bHovered = fSqDistance < (fRadius * fRadius);
	bHovered = bXHover && bYHover;

	// If the hovered state has changed, update the sprite texture accordingly
	if (bHovered != bCurrentHoverState)
	{
		setTexture(bHovered ? tHoveredTexture : tBaseTexture);
	}
}
