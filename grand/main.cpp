#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Network.hpp>

#include <iostream>

#include <vector>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <functional>
#include <cmath>

std::set<sf::Keyboard::Key> const keys{
	sf::Keyboard::A,
	sf::Keyboard::B,
	sf::Keyboard::C,
	sf::Keyboard::D,
	sf::Keyboard::E,
	sf::Keyboard::F,
	sf::Keyboard::G,
	sf::Keyboard::H,
	sf::Keyboard::I,
	sf::Keyboard::J,
	sf::Keyboard::K,
	sf::Keyboard::L,
	sf::Keyboard::M,
	sf::Keyboard::N,
	sf::Keyboard::O,
	sf::Keyboard::P,
	sf::Keyboard::Q,
	sf::Keyboard::R,
	sf::Keyboard::S,
	sf::Keyboard::T,
	sf::Keyboard::U,
	sf::Keyboard::V,
	sf::Keyboard::W,
	sf::Keyboard::X,
	sf::Keyboard::Y,
	sf::Keyboard::Z
};

int const attempt = 0, success = 1;

struct signal
{
	int last_signal_time = -1000000;
	int last_signal_type = attempt;
	int delay = 0;

	signal(int delay1)
	{
		delay = delay1;
	}

	void update_cache(int time_stamp, int new_type)
	{
		if (time_stamp > last_signal_time)
		{
			last_signal_time = time_stamp;
			last_signal_type = new_type;
		}
	}

	void draw(sf::RenderTarget &window, sf::RectangleShape &rect, int current_time)
	{
		if (last_signal_time + delay >= current_time)
		{
			if (last_signal_type == attempt)
				rect.setFillColor(sf::Color::Yellow);
			else
				rect.setFillColor(sf::Color::Green);
		}
		else
			rect.setFillColor(sf::Color::Black);
		window.draw(rect);
	}
};

sf::Packet& operator<<(sf::Packet& packet, signal const &s)
{
	packet << s.last_signal_type;
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, signal &s)
{
	packet >> s.last_signal_type;
	return packet;
}

struct socket_sender
{
	sf::UdpSocket socket;
	sf::IpAddress addr;
	unsigned short port;

	socket_sender(sf::IpAddress addr1, unsigned short port1)
	{
		socket.setBlocking(false);
		addr = addr1;
		port = port1;
	}

	void send(sf::Packet &p)
	{
		socket.send(p, addr, port);
	}
};

struct socket_receiver
{
	sf::UdpSocket client;
	unsigned short port;

	socket_receiver(unsigned short port1)
	{
		client.setBlocking(false);
		port = port1;
		client.bind(port);
	}

	bool receive(sf::Packet &p)
	{
		sf::IpAddress remote_addr;
		unsigned short remote_port;
		auto code = client.receive(p, remote_addr, remote_port);
		return code == sf::Socket::Status::Done;
	}
};

int main(int argc, char **argv)
{
	if (argc != 11)
		return EXIT_FAILURE;

	auto m = sf::VideoMode::getFullscreenModes()[0];
	unsigned int const width = m.width, height = m.height;

	sf::RenderWindow window{
			sf::VideoMode{width, height, m.bitsPerPixel},
			"100000000000",
			sf::Style::Fullscreen,
			sf::ContextSettings{0, 0, 8}
	};
	window.setVerticalSyncEnabled(true);

	int const sends_per_second = 20;

	sf::Clock clock;
	sf::Time stepTime = sf::seconds(1.0 / sends_per_second);
	sf::Time time = stepTime;
	int current_time = 0;

	sf::RectangleShape rectangle({1.0f, 1.0f});


	signal send_signal(1);
	signal receive_signal1(10);
	signal receive_signal2(10);
	signal buf_signal(10);

	socket_sender sender1(argv[2], std::stoi(argv[3]));
	socket_receiver receiver1(std::stoi(argv[1]));

	socket_sender sender2(argv[5], std::stoi(argv[6]));
	socket_receiver receiver2(std::stoi(argv[4]));

	std::vector<char> success_keys{argv[7][0], argv[8][0], argv[9][0], argv[10][0]};
	std::sort(success_keys.begin(), success_keys.end());

	std::set<char> pressed_keys;
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
			switch (event.type)
			{
				case sf::Event::KeyPressed:
					if (keys.find(event.key.code) != keys.end())
						pressed_keys.insert(event.key.code - sf::Keyboard::A + 'A');
					break;

				case sf::Event::KeyReleased:
					if (keys.find(event.key.code) != keys.end())
						pressed_keys.erase(event.key.code - sf::Keyboard::A + 'A');
					break;

				case sf::Event::Closed:
					window.close();
					break;

				default:
					break;
			}


		if (!pressed_keys.empty())
		{
			if (std::equal(pressed_keys.begin(), pressed_keys.end(),
						success_keys.begin(), success_keys.end()))
				send_signal.update_cache(current_time, success);
			else
				send_signal.update_cache(current_time, attempt);
		}

		time += clock.restart();
		while (time > stepTime)
		{
			if (send_signal.last_signal_time == current_time)
			{
				sf::Packet packet;
				packet << send_signal;
				sender1.send(packet);
				sender2.send(packet);
			}

			++current_time;
			time -= stepTime;
		}

		sf::Packet packet;
		while (receiver1.receive(packet))
		{
			packet >> buf_signal;
			receive_signal1.update_cache(current_time, buf_signal.last_signal_type);
		}
		while (receiver2.receive(packet))
		{
			packet >> buf_signal;
			receive_signal2.update_cache(current_time, buf_signal.last_signal_type);
		}


		window.clear();

		auto view = sf::View{{0, 0, 1, 1}};
		view.setViewport(sf::FloatRect{0.2f, 0.2f, 0.2f, 0.2f});
		window.setView(view);
		receive_signal1.draw(window, rectangle, current_time);

		view.setViewport(sf::FloatRect{0.6f, 0.2f, 0.2f, 0.2f});
		window.setView(view);
		receive_signal2.draw(window, rectangle, current_time);

		view.setViewport(sf::FloatRect{0.4f, 0.6f, 0.2f, 0.2f});
		window.setView(view);
		send_signal.draw(window, rectangle, current_time);

		window.display();
	}

	return 0;
}
