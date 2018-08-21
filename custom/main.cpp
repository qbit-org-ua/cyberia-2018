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
	std::deque<char> s;
	int current_time = 0;

	signal(int size)
	{
		for (int i = 0; i < size; ++i)
			s.push_back(' ');
	}

	void set_current_time(int t)
	{
		current_time = t;
		for (auto it = s.begin(); it != s.end(); ++it)
			*it = false;
	}

	void update_cache(int time_stamp, char c)
	{
		int index = current_time - time_stamp;
		if (index >= 0 && index < static_cast<int>(s.size()))
			s[index] = c;
	}

	void iterate(char c)
	{
		s.push_back(c);
		s.pop_front();
		++current_time;
	}

	void draw(sf::RenderTarget &window, sf::Text &text)
	{
		std::vector<char> str_vec;
		auto it = s.begin();
		int i = 0;
		while (it != s.end())
		{
			str_vec.push_back(*it);
			++it;
			++i;
			if (i % 75 == 0)
				str_vec.push_back('\n');
		}
		text.setString(std::string(str_vec.begin(), str_vec.end()));
		text.setOrigin(text.getLocalBounds().width / 2, text.getLocalBounds().height / 2);
		text.setPosition(window.getView().getCenter());
		window.draw(text);
	}
};

sf::Packet& operator<<(sf::Packet& packet, signal const &s)
{
	packet << s.current_time;
	for (auto it = s.s.begin(); it != s.s.end(); ++it)
		packet << static_cast<sf::Int8>(*it);
	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, signal &s)
{
	packet >> s.current_time;
	for (auto it = s.s.begin(); it != s.s.end(); ++it)
	{
		sf::Int8 code;
		packet >> code;
		*it = static_cast<char>(code);
	}
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

struct decoder
{
	int pressed_n = 0;
	unsigned int current_code = 0;
	std::function<void(char)> callback;

	decoder(std::function<void(char)> callback1)
	{
		callback = callback1;
	}

	void process(unsigned int key)
	{
		current_code <<= 1;
		current_code |= key;
		++pressed_n;
		if (pressed_n == 5)
		{
			char c = 'A' - (32 - 26) + current_code;
			callback(c);
			pressed_n = 0;
			current_code = 0;
		}
	}
};

int main(int argc, char **argv)
{
	if (argc != 4)
		return EXIT_FAILURE;

	int const buf_len = 750;
	int const width = 640, height = 480;

	sf::RenderWindow window{
			sf::VideoMode{width, height},
			"Graphs",
			sf::Style::Default ^ sf::Style::Resize,
			sf::ContextSettings{0, 0, 8}
	};
	window.setVerticalSyncEnabled(true);

	int const sends_per_second = 5;

	sf::Clock clock;
	sf::Time stepTime = sf::seconds(1.0 / sends_per_second);
	sf::Time time = stepTime;

	sf::Font font;
	if (!font.loadFromFile("../UbuntuMono-R.ttf"))
	{
		return EXIT_FAILURE;
	}

	sf::Text text("", font, 14);
	text.setLineSpacing(1.15f);
	text.setLetterSpacing(1.1f);

	signal send_signal(buf_len);
	signal receive_signal(buf_len);
	signal buf_signal(buf_len);

	socket_sender sender(argv[2], std::stoi(argv[3]));
	socket_receiver receiver(std::stoi(argv[1]));

	decoder keyboard_decoder([&send_signal](char c) { send_signal.iterate(c); });

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
			switch (event.type)
			{
				case sf::Event::KeyPressed:
				{
					switch (event.key.code)
					{
						case sf::Keyboard::Num0:
						case sf::Keyboard::Num1:
							keyboard_decoder.process(event.key.code - sf::Keyboard::Num0);
							break;

						default:
							break;
					}
					break;
				}

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

			sf::Packet packet;
			packet << send_signal;
			sender.send(packet);
		}

		sf::Packet packet;
		while (receiver.receive(packet))
		{
			packet >> buf_signal;
			if (buf_signal.current_time == 0
					|| buf_signal.current_time > receive_signal.current_time)
				receive_signal = buf_signal;
		}


		window.clear();

		auto view = sf::View{{0, 0, 640, 240}};
		view.setViewport(sf::FloatRect{0.0f, 0.0f, 1.0f, 0.5f});
		window.setView(view);
		text.setFillColor(sf::Color::White);
		text.setOutlineColor(sf::Color::White);
		send_signal.draw(window, text);

		view.setViewport(sf::FloatRect{0.0f, 0.5f, 1.0f, 0.5f});
		window.setView(view);
		text.setFillColor(sf::Color::Yellow);
		text.setOutlineColor(sf::Color::Yellow);
		receive_signal.draw(window, text);

		window.display();
	}

	return 0;
}
