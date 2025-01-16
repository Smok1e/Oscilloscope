#pragma warning(disable: 6385)

#include <iostream>
#include <chrono>
#include <deque>
#include <algorithm>
#include <cmath>

#include <SFML/Graphics.hpp>

#include <OpenALUtils/SoundBuffer.hpp>
#include <OpenALUtils/SoundDevice.hpp>
#include <OpenALUtils/SoundCaptureDevice.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

using namespace std::chrono_literals;

//=================================================

sf::Color Interpolate(sf::Color a, sf::Color b, float t);

void DrawLine(
	sf::RenderTarget& target, 
	const sf::Vector2f& a, 
	const sf::Vector2f& b, 
	sf::Color color_a, 
	sf::Color color_b, 
	float thickness /*= 2.5*/
);

//=================================================

class Main
{
public:
	Main() = default;

	void start();

private:
	enum class VisualizationMode 
	{
		Points,
		Lines,
		
		Default = Points
	} m_visualization_mode {VisualizationMode::Default};

	enum class SignalSource
	{
		Time,
		LeftChannel,
		RightChannel,

		DefaultX = LeftChannel,
		DefaultY = RightChannel
	}
		m_x_axis_source {SignalSource::DefaultX}, 
		m_y_axis_source {SignalSource::DefaultY};

	enum class SignalInterpretation
	{
		Cartesian,
		Polar,

		Default = Cartesian
	}
		m_signal_interpretation {SignalInterpretation::Default};

	alu::SoundDevice          m_sound_device     {};
	alu::SoundCaptureDevice   m_capture_device   {};
	alu::SoundBuffer          m_sound_buffer     {};

	std::chrono::high_resolution_clock::time_point m_last_render_time {};
    int                                            m_fps {};

	sf::ContextSettings       m_context_settings {};
	sf::RenderWindow          m_render_window    {};
	sf::RenderTexture         m_render_texture   {};
							  				       
	std::deque<sf::Vector2f>  m_points     {};
	int                       m_max_points {1000};

	int                                   m_recording_interval_ms {50};
	std::chrono::system_clock::time_point m_recording_start_time  {};
	size_t                                m_last_processed_sample {0};

	float m_x_amplification  {1000.f};
	float m_y_amplification  {1000.f};
	float m_thickness        {4.f};
	float m_distortion_power {1.f};
	int   m_glow_radius      {6};
	bool  m_vertical_sync    {true};

	sf::Sprite m_sprite {};

	sf::Color m_background_color {  0,   4,   5};
	sf::Color m_color            {177, 224, 255};

	sf::Shader m_postfx_shader {};

	int m_interface_width { 500 };

	void loadShaders();

	double getSourceSignal(SignalSource source, size_t sample);
	void interpretSignal(
		double (&signal)[2],
		double (&result)[2]
	);

	void processSamples();
	void commitSamples();
	void renderPoints();

	void resize(sf::Vector2u window_size, unsigned interface_width);
	void processInterface();

	void onEvent (const sf::Event& event);
	void onClose (const sf::Event& event);
	void onResize(const sf::Event& event);

	template<typename ItemT, size_t ItemCount>
	static int ComboBoxDefaultIndex(
		std::pair<ItemT, const char*> (&items)[ItemCount],
		ItemT default_value
	);

	template<typename ItemT, size_t ItemCount>
	static ItemT ComboBox(
		const char* name, 
		std::pair<ItemT, const char*> (&items)[ItemCount], 
		int* selected_item_index
	);

	static void ColorEdit(const char* name, sf::Color* color);

};

//================================================= Main loop

void Main::start()
{
	m_capture_device.setBuffer(m_sound_buffer);
	m_capture_device.setProcessingInterval(std::chrono::milliseconds(m_recording_interval_ms));

	const sf::Vector2u window_size(1000 + m_interface_width, 1000);
	m_render_window.create(sf::VideoMode(window_size.x, window_size.y), "Oscilloscope");
	m_render_window.setVerticalSyncEnabled(m_vertical_sync);

	m_context_settings.antialiasingLevel = 8;
	m_render_texture.create(window_size.x - m_interface_width, window_size.y, m_context_settings);
	m_sprite.setTexture(m_render_texture.getTexture());

	loadShaders();

	if (!ImGui::SFML::Init(m_render_window))
	{
		std::cerr << "ImGUI-SFML initialization failure" << std::endl;
		return;
	}

	sf::Clock delta_clock;

	commitSamples();
	while (m_render_window.isOpen())
	{
		sf::Event event;
		while (m_render_window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(m_render_window, event);
			onEvent(event);
		}

		processSamples();

		// Rendering
		ImGui::SFML::Update(m_render_window, delta_clock.restart());

		processInterface();

		m_render_texture.clear(m_background_color);

		renderPoints();
		m_render_texture.display();
	
		m_render_window.clear();
		m_sprite.setPosition(m_interface_width, 0);

		m_postfx_shader.setUniform("distortion_power", m_distortion_power);
		m_postfx_shader.setUniform("texture_data",     sf::Shader::CurrentTexture);
		m_postfx_shader.setUniform("texture_size",     sf::Glsl::Vec2(m_render_texture.getSize()));
		m_postfx_shader.setUniform("texture_offset",   sf::Glsl::Vec2(m_sprite.getPosition()));
		m_postfx_shader.setUniform("background_color", sf::Glsl::Vec4(m_background_color));
		m_postfx_shader.setUniform("glow_radius",      m_glow_radius);

		m_render_window.draw(m_sprite, &m_postfx_shader);

		ImGui::SFML::Render(m_render_window);
		m_render_window.display();

		auto current_time = std::chrono::system_clock::now();
        auto render_time = current_time - m_last_render_time;
		m_last_render_time = current_time;

		m_fps = 1.0 / std::chrono::duration_cast<std::chrono::duration<double>>(render_time).count();
	}
}

void Main::loadShaders()
{
	m_postfx_shader.loadFromFile("resources/shaders/postfx.frag", sf::Shader::Fragment);
}

//=================================================	Signal processing

double Main::getSourceSignal(SignalSource source, size_t sample)
{
	auto format = m_sound_buffer.getFormat();
	auto channels = m_sound_buffer.getSamples() + (format.getSampleSize() * sample);

	double signal = 0;
	switch (source)
	{
		case SignalSource::LeftChannel:
			signal = format.normalizeSample(channels);
			break;

		case SignalSource::RightChannel:
			signal = format.normalizeSample(channels + format.getBytesPerSample());
			break;

		case SignalSource::Time:
			signal = static_cast<double>(sample) / m_sound_buffer.getSampleCount();
			break;
	}

	// Mapping from [0, 1] to [-1, 1]
	return 2.0 * signal - 1.0;
}

void Main::interpretSignal(
	double (&signal)[2],
	double (&result)[2]
)
{
	switch (m_signal_interpretation)
	{
		case SignalInterpretation::Cartesian:
			result[0] = signal[0];
			result[1] = signal[1];
			break;

		case SignalInterpretation::Polar:
			result[0] = signal[1] * cos(signal[0]);
			result[1] = signal[1] * sin(signal[0]);
			break;
	}
}

void Main::processSamples()
{
	// Calculating sample corresponding to current moment
	auto   recording_time = std::chrono::high_resolution_clock::now() - m_recording_start_time;
	size_t current_sample = m_sound_buffer.getSampleRate() * std::chrono::duration<double>(recording_time).count();
	auto   format = m_capture_device.getFormat();

	auto center = .5f * sf::Vector2f(m_render_texture.getSize());

	double signal[2] = {}, result[2] = {};

	for (
		int sample = m_last_processed_sample; 
		sample < std::min(m_sound_buffer.getSampleCount(), current_sample); 
		sample++
	)
	{
		signal[0] = getSourceSignal(m_x_axis_source, sample);
		signal[1] = getSourceSignal(m_y_axis_source, sample);
		interpretSignal(signal, result);

		m_points.emplace_front(
			center.x + m_x_amplification * result[0],
			center.y - m_y_amplification * result[1]
		);
	}

	m_last_processed_sample = current_sample;

	// Deleting extra points
	if (m_points.size() > m_max_points)
		m_points.erase(m_points.end() - (m_points.size() - m_max_points) - 1, m_points.end());

	// Restarting recording if needed
	if (recording_time > std::chrono::milliseconds(m_recording_interval_ms))
		commitSamples();
}

void Main::commitSamples()
{
	if (m_capture_device.isRecording())
		m_capture_device.commit();

	else
		m_capture_device.start();

	m_recording_start_time = std::chrono::high_resolution_clock::now();
	m_last_processed_sample = 0;
}

//=================================================	Rendering

void Main::renderPoints()
{
	auto point_color = [this](size_t i) -> sf::Color
	{
		return sf::Color(
			m_color.r,
			m_color.g,
			m_color.b,
			0xFF * std::pow(1.f - static_cast<float>(i) / m_points.size(), 2)
		);
	};

	switch (m_visualization_mode)
	{
		case VisualizationMode::Points:
		{
			static sf::CircleShape circle;
			circle.setRadius(.5f * m_thickness);
			circle.setOrigin(.5f * m_thickness, .5f * m_thickness);

			for (size_t i = 0; i < m_points.size(); i++)
			{
				circle.setFillColor(point_color(i));
				circle.setPosition(m_points[i]);

				m_render_texture.draw(circle);
			}
			
			break;
		}

		case VisualizationMode::Lines:
		{
			for (size_t i = 0; i < m_points.size() - 1; i++)
			{
				DrawLine(
					m_render_texture,
					m_points[i],
					m_points[i + 1],
					point_color(i),
					point_color(i + 1),
					m_thickness
				);
			}

			break;
		}
	}
}

//================================================= GUI

void Main::resize(sf::Vector2u window_size, unsigned interface_width)
{
	m_render_window.setView(sf::View(sf::FloatRect(0, 0, window_size.x, window_size.y)));

	m_render_texture.create(window_size.x - m_interface_width, window_size.y, m_context_settings);
	m_sprite.setTexture(m_render_texture.getTexture(), true);	
	m_sprite.setPosition(interface_width, 0);

	m_interface_width = interface_width;
}

void Main::processInterface()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(m_interface_width, m_render_texture.getSize().y));
	ImGui::Begin(
		"Options", 
		nullptr, 
		ImGuiWindowFlags_NoMove
	);

	auto gui_window_size = ImGui::GetWindowSize();
	if (gui_window_size.x != m_interface_width)
		resize(m_render_window.getSize(), gui_window_size.x);

	// Capture settings
	ImGui::SeparatorText("Capture settings");

	static int capture_device_index = 0;
	const auto& capture_devices = alu::SoundCaptureDevice::GetDevices();
	if (ImGui::BeginCombo("Capture device", capture_devices[capture_device_index].c_str()))
	{
		for (size_t i = 0; i < std::size(capture_devices); i++)
		{
			bool selected = i == capture_device_index;
			if (ImGui::Selectable(capture_devices[i].c_str(), selected))
				m_capture_device.create(capture_devices[capture_device_index = i]);

			if (selected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	if (ImGui::SliderInt("Recording time", &m_recording_interval_ms, 10, 300, "%d ms"))
		m_capture_device.setProcessingInterval(std::chrono::milliseconds(m_recording_interval_ms));

	// Visualization settings
	ImGui::SeparatorText("Visualization");

	static std::pair<VisualizationMode, const char*> visualization_modes[] = {
		{VisualizationMode::Points, "Points"},
		{VisualizationMode::Lines,  "Lines" }
	};

	static int visualization_mode_index = ComboBoxDefaultIndex(
		visualization_modes, VisualizationMode::Default
	);

	m_visualization_mode = ComboBox("Visualization mode", visualization_modes, &visualization_mode_index);

	ImGui::SliderFloat("Thickness",             &m_thickness,        .5f, 10.f );
	ImGui::SliderInt  ("Points limit",          &m_max_points,        1,  10000);
	ImGui::SliderFloat("Distortion power",      &m_distortion_power, .1f, 5.f  );
	ImGui::SliderInt  ("Glow radius",           &m_glow_radius,       0,  20   );

	ColorEdit("Color",            &m_color           );
	ColorEdit("Background color", &m_background_color);

	if (ImGui::Checkbox("Vertical synchronization", &m_vertical_sync))
		m_render_window.setVerticalSyncEnabled(m_vertical_sync);

	ImGui::Text("FPS: %d", m_fps);

	// Signal source settings
	ImGui::SeparatorText("Signal source");

	static std::pair<SignalSource, const char*> signal_sources[] = {
		{SignalSource::LeftChannel,  "Left channel" },
		{SignalSource::RightChannel, "Right channel"},
		{SignalSource::Time,         "Time"         }
	};

	static int x_axis_source_index = ComboBoxDefaultIndex(
		signal_sources,
		SignalSource::DefaultX
	);

	static int y_axis_source_index = ComboBoxDefaultIndex(
		signal_sources,
		SignalSource::DefaultY
	);

	m_x_axis_source = ComboBox(
		"X signal source",
		signal_sources, 
		&x_axis_source_index
	);

	m_y_axis_source = ComboBox(
		"Y signal source",
		signal_sources, 
		&y_axis_source_index
	);

	// Signal interpretation settings
	ImGui::SeparatorText("Signal interpretation");

	static std::pair<SignalInterpretation, const char*> signal_interpretation[] = {
		{SignalInterpretation::Cartesian, "Cartesian"},
		{SignalInterpretation::Polar,     "Polar"    }
	};

	static int signal_interpretation_index = ComboBoxDefaultIndex(
		signal_interpretation,
		SignalInterpretation::Default
	);

	m_signal_interpretation = ComboBox(
		"Signal interpretation", 
		signal_interpretation, 
		&signal_interpretation_index
	);

	static bool sync_amplification = true;
	ImGui::Checkbox("Synchronize amplification", &sync_amplification);

	if (ImGui::SliderFloat("X signal amplification", &m_x_amplification, 0.f, 10000.f))
		if (sync_amplification)
			m_y_amplification = m_x_amplification;

	if (ImGui::SliderFloat("Y signal amplification", &m_y_amplification, 0.f, 10000.f))
		if (sync_amplification)
			m_x_amplification = m_y_amplification;

	// Debugging
	ImGui::SeparatorText("Debugging");

	if (ImGui::Button("Reload shaders"))
		loadShaders();

	ImGui::End();
}

//================================================= Events

void Main::onEvent(const sf::Event& event)
{
	switch (event.type)
	{
		case sf::Event::Closed:
			onClose(event);
			break;

		case sf::Event::Resized:
			onResize(event);
			break;
	}
}

void Main::onClose(const sf::Event& event)
{
	m_render_window.close();
}

void Main::onResize(const sf::Event& event)
{
	static const unsigned minimum_size = m_interface_width + 100;
	if (event.size.width < minimum_size)
	{
		m_render_window.setSize(sf::Vector2u(minimum_size, event.size.height));
		return;
	}

	resize(sf::Vector2u(event.size.width, event.size.height), m_interface_width);
}

//================================================= GUI helpers

template<typename ItemT, size_t ItemCount>
int Main::ComboBoxDefaultIndex(
	std::pair<ItemT, const char*> (&items)[ItemCount],
	ItemT default_value
)
{
	return std::distance(
		std::begin(items),
		std::find_if(
			std::begin(items),
			std::end(items),
			[default_value](const std::pair<ItemT, const char*>& item) -> bool
			{
				return item.first == default_value;
			}
		)
	);
}

template<typename ItemT, size_t ItemCount>
ItemT Main::ComboBox(
	const char* name, 
	std::pair<ItemT, const char*> (&items)[ItemCount], 
	int* selected_item_index
)
{
	if (ImGui::BeginCombo(name, items[*selected_item_index].second))
	{
		for (size_t i = 0; i < std::size(items); i++)
		{
			bool selected = i == *selected_item_index;
			if (ImGui::Selectable(items[i].second, selected))
				*selected_item_index = i;

			if (selected);
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	return items[*selected_item_index].first;
}

void Main::ColorEdit(const char* name, sf::Color* color)
{
	float color_normalized[] = {
		static_cast<float>(color->r) / 0xFF,
		static_cast<float>(color->g) / 0xFF,
		static_cast<float>(color->b) / 0xFF
	};

	if (ImGui::ColorEdit3(name, color_normalized))
		*color = sf::Color(
			0xFF * color_normalized[0],
			0xFF * color_normalized[1],
			0xFF * color_normalized[2]
		);
}

//=================================================

int main()
{
	Main instance;
	instance.start();
}

//=================================================

sf::Color Interpolate(sf::Color a, sf::Color b, float t)
{
	return sf::Color(
		a.r + t * (b.r - a.r),
		a.g + t * (b.g - a.g),
		a.b + t * (b.b - a.b),
		a.a + t * (b.a - a.a)
	);
}

template<typename T>
double VectorLength(const sf::Vector2<T>& vec)
{
	return std::sqrt(vec.x*vec.x + vec.y*vec.y);
}

template<typename T>
sf::Vector2<T> NormalizeVector(const sf::Vector2<T>& vec)
{
	auto length = VectorLength(vec);
	return sf::Vector2<T>(
		vec.x / length, 
		vec.y / length
	);								
}

template<typename T>
sf::Vector2<T> Perpendicular(const sf::Vector2<T>& vec)
{
	return sf::Vector2<T>(
		vec.y,
		-vec.x
	);
}

void DrawLine(
	sf::RenderTarget& target, 
	const sf::Vector2f& a, 
	const sf::Vector2f& b, 
	sf::Color color_a, 
	sf::Color color_b, 
	float thickness /*= 2.5*/
)
{
	sf::Vector2f govno = Perpendicular(NormalizeVector(b - a)) * thickness * .5f;
	sf::Vertex points[] = {
		sf::Vertex(a - govno, color_a),
		sf::Vertex(a + govno, color_a),
		sf::Vertex(b - govno, color_b),
		sf::Vertex(b + govno, color_b),
	};

	target.draw(points, 4, sf::TriangleStrip);
}

//=================================================
