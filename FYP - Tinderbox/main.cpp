#include <iostream>
#include <SFML/Graphics.hpp>

int main()
{
	sf::RenderWindow wWindow(sf::VideoMode(800, 800), "Tinderbox");
	sf::Event eWinEvent;

	while (wWindow.isOpen())
	{
		while (wWindow.pollEvent(eWinEvent))
		{
			switch (eWinEvent.type)
			{
				case sf::Event::Closed:
					wWindow.close();
					break;
			}
		}
	}
}


