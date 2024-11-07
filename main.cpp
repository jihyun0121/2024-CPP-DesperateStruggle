#include <SFML/Graphics.hpp>

using namespace sf;
using namespace std;

#define WINDOW_WIDTH	1200
#define WINDOW_HEIGHT	800

int main() {
    sf::RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Window");

    while (window.isOpen()) {
		Event event;
        while (window.pollEvent(event)) {
			if (event.type == Event::Closed)
				window.close();
        }

        window.clear(Color::White);

        window.display();
    }

    return 0;
}