#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
    sf::RenderWindow window(sf::VideoMode(400, 200), "Thông báo");

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) return -1;

    sf::Text message("Ban co muon tiep tuc?", font, 20);
    message.setPosition(50, 30);
    message.setFillColor(sf::Color::Black);

    sf::RectangleShape yesButton(sf::Vector2f(100, 40));
    yesButton.setFillColor(sf::Color::Green);
    yesButton.setPosition(60, 100);

    sf::Text yesText("Yes", font, 20);
    yesText.setPosition(90, 110);
    yesText.setFillColor(sf::Color::White);

    sf::RectangleShape noButton(sf::Vector2f(100, 40));
    noButton.setFillColor(sf::Color::Red);
    noButton.setPosition(220, 100);

    sf::Text noText("No", font, 20);
    noText.setPosition(255, 110);
    noText.setFillColor(sf::Color::White);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (yesButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    std::cout << "Nguoi dung chon: Yes\n";
                    window.close();
                }
                if (noButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                    std::cout << "Nguoi dung chon: No\n";
                    window.close();
                }
            }
        }

        window.clear(sf::Color::White);
        window.draw(message);
        window.draw(yesButton);
        window.draw(yesText);
        window.draw(noButton);
        window.draw(noText);
        window.display();
    }

    return 0;
}
