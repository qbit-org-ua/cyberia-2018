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
	int current_time = 0;

	signal(int size)
	{
		for (int i = 0; i < size; ++i)
			s.push_back(false);
	}

	void set_current_time(int t)
	{
		current_time = t;
		for (auto it = s.begin(); it != s.end(); ++it)
			*it = false;
	}

	void update_cache(int time_stamp)
	{
		int index = current_time - time_stamp;
		if (index >= 0 && index < static_cast<int>(s.size()))
			s[index] = true;
	}

	void iterate()
	{
		s.push_front(false);
		s.pop_back();
		++current_time;
	}

	void draw(sf::RenderTarget &window, sf::RectangleShape &line)
	{
		int i = 0;
		float const width = window.getView().getSize().x;
		for (auto it = s.rbegin(); it != s.rend() && i < width; ++it, ++i)
			if (*it)
			{
				line.setPosition({static_cast<float>(i), 0});
				window.draw(line);
			}
	}
};

sf::Packet& operator<<(sf::Packet& packet, signal const &s)
{
	packet << s.current_time;
	for (auto it = s.s.begin(); it != s.s.end(); ++it)
		packet << *it;
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, signal &s)
{
	packet >> s.current_time;
	for (auto it = s.s.begin(); it != s.s.end(); ++it)
		packet >> *it;
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
	if (argc != 4)
		return EXIT_FAILURE;

	int const width = 640, height = 480;

	sf::RenderWindow window{
			sf::VideoMode{width, height},
			"Graphs",
			sf::Style::Default ^ sf::Style::Resize,
			sf::ContextSettings{0, 0, 8}
	};
	window.setVerticalSyncEnabled(true);

	int const signals_per_second = 100;
	int const sends_per_second = 5;

	sf::Clock clock;
	sf::Time time;
	sf::Time stepTime = sf::seconds(1.0 / signals_per_second);

	//sf::Font font;
	//if (!font.loadFromFile("/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-B.ttf"))
	//{
	//	return EXIT_FAILURE;
	//}

	//sf::Clock fps_clock;
	//std::deque<sf::Time> fps_timestamps;
	//sf::Text fps_text("", font, 12);
	//fps_text.setColor(sf::Color::Red);
	//fps_text.setOutlineColor(sf::Color::Red);

	sf::RectangleShape line({1.0f, height});

	signal send_signal(width);
	signal receive_signal(width + signals_per_second);
	receive_signal.set_current_time(1000000000);
	signal buf_signal(width);

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

			send_signal.iterate();
			receive_signal.iterate();

			if (is_pressed)
				send_signal.update_cache(send_signal.current_time);

			if (send_signal.current_time % (signals_per_second / sends_per_second) == 0)
			{
				sf::Packet packet;
				packet << send_signal;
				sender.send(packet);
			}

			int const signal_size = static_cast<int>(receive_signal.s.size());
			sf::Packet packet;
			while (receiver.receive(packet))
			{
				packet >> buf_signal;
				for (int ts = 0; ts < static_cast<int>(buf_signal.s.size()); ++ts)
					if (buf_signal.s[ts] == true)
					{
						int time_stamp = buf_signal.current_time - ts;
						if (time_stamp < receive_signal.current_time - 2 * signal_size
								|| time_stamp > receive_signal.current_time + signal_size)
							receive_signal.set_current_time(time_stamp);
						receive_signal.update_cache(time_stamp);
					}
			}
		}

		//fps_timestamps.push_back(fps_clock.getElapsedTime());
		//while (fps_timestamps.size() > 0
		//		&& fps_timestamps.back() - fps_timestamps.front() > sf::seconds(1))
		//	fps_timestamps.pop_front();
		//fps_text.setString(std::to_string(fps_timestamps.size()) + " fps");


		window.clear();

		auto view = sf::View{{0, 0, 640, 480}};
		view.setViewport(sf::FloatRect{0.0f, 0.1f, 1.0f, 0.35f});
		window.setView(view);
		line.setFillColor(sf::Color::White);
		send_signal.draw(window, line);

		view.setViewport(sf::FloatRect{0.0f, 0.55f, 1.0f, 0.35f});
		window.setView(view);
		line.setFillColor(sf::Color::Yellow);
		receive_signal.draw(window, line);

		window.setView(window.getDefaultView());
	
		//window.draw(fps_text);

		window.display();
	}

	return 0;
}
