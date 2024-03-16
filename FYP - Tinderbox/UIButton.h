#pragma once

#include <SFML/Graphics.hpp>

class UIButton : public sf::Sprite
{
public:
	void Initialize(sf::Vector2f aiPosition, std::string asBasePath, std::string asHoveredPath);
	void Tick(sf::Vector2i aiMousePos);

	template <typename F>
	bool OnClick(F afFunction)
	{
		if (bHovered)
		{
			afFunction();
		}
		return bHovered;
	}

private:
	float fRadius = 32;

	sf::Texture tBaseTexture;
	sf::Texture tHoveredTexture;

	bool bHovered = false;
};

