#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Network.hpp>

#include <iostream>

#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <string>
#include <functional>
#include <cmath>

struct signal
{
	std::deque<bool> s;

	signal(int size)
	{
		for (int i = 0; i < size; ++i)
			s.push_back(false);
	}

	void update_cache(bool is_pressed)
	{
		s.push_back(is_pressed);
		s.pop_front();
	}

	void draw(sf::RenderTarget &window, sf::RectangleShape &line)
	{
		int i = 0;
		for (auto it = s.rbegin(); it != s.rend(); ++it, ++i)
			if (*it)
			{
				line.setPosition({static_cast<float>(i), 0});
				window.draw(line);
			}
	}
};

bool check_if_connected(sf::Socket::Status status)
{
	return status == sf::Socket::Status::Done || status == sf::Socket::Status::NotReady;
}

struct socket_sender
{
	sf::TcpSocket socket;
	bool is_connected = false;
	sf::IpAddress addr;
	unsigned short port;

	socket_sender(sf::IpAddress addr1, unsigned short port1)
	{
		socket.setBlocking(false);
		addr = addr1;
		port = port1;
	}

	void send(bool is_pressed)
	{
		if (!is_connected)
			socket.connect(addr, port);
		auto code = socket.send(&is_pressed, 1);
		is_connected = check_if_connected(code);
	}
};

struct socket_receiver
{
	sf::TcpListener listener;
	sf::TcpSocket client;
	bool is_connected = false;
	unsigned short port;

	socket_receiver(unsigned short port1)
	{
		listener.setBlocking(false);
		client.setBlocking(false);
		port = port1;
		listener.listen(port);
	}

	bool receive()
	{
		if (!is_connected)
			listener.accept(client);
		bool is_pressed = false;
		size_t received = 0;
		auto code = client.receive(&is_pressed, 1, received);
		is_connected = check_if_connected(code);
		return is_pressed;
	}
};

int main(int argc, char **argv)
{
	if (argc != 4)
		return EXIT_FAILURE;

	int const width = 640, height = 480;

	sf::RenderWindow window{
			sf::VideoMode{width, height},
			"Graphs",
			sf::Style::Default ^ sf::Style::Resize,
			sf::ContextSettings{0, 0, 8}
	};
	//window.setVerticalSyncEnabled(true);

	sf::Clock clock;
	sf::Time time;
	sf::Time stepTime = sf::seconds(1.0 / 100);

	sf::Font font;
	if (!font.loadFromFile("/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-B.ttf"))
	{
		return EXIT_FAILURE;
	}

	sf::Clock fps_clock;
	std::deque<sf::Time> fps_timestamps;
	sf::Text fps_text("", font, 12);
	fps_text.setColor(sf::Color::Red);
	//fps_text.setOutlineColor(sf::Color::Red);

	sf::RectangleShape line({1.0f, height});

	signal send_signal(width);
	signal receive_signal(width);

	socket_sender sender(argv[2], std::stoi(argv[3]));
	socket_receiver receiver(std::stoi(argv[1]));

	while (window.isOpen())
	{
		bool is_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

		sf::Event event;
		while (window.pollEvent(event))
			switch (event.type)
			{
				case sf::Event::Closed:
					window.close();
					break;

				default:
					break;
			}


		time += clock.restart();
		while (time > stepTime)
		{
			time -= stepTime;
			send_signal.update_cache(is_pressed);
			sender.send(is_pressed);
			receive_signal.update_cache(receiver.receive());
		}

		fps_timestamps.push_back(fps_clock.getElapsedTime());
		while (fps_timestamps.size() > 0
				&& fps_timestamps.back() - fps_timestamps.front() > sf::seconds(1))
			fps_timestamps.pop_front();
		fps_text.setString(std::to_string(fps_timestamps.size()) + " fps");


		window.clear();

		auto view = sf::View{{0, 0, 640, 480}};
		view.setViewport(sf::FloatRect{0.0f, 0.1f, 1.0f, 0.35f});
		window.setView(view);
		send_signal.draw(window, line);

		view.setViewport(sf::FloatRect{0.0f, 0.55f, 1.0f, 0.35f});
		window.setView(view);
		receive_signal.draw(window, line);

		window.setView(window.getDefaultView());
	
		window.draw(fps_text);

		window.display();
	}

	return 0;
}
