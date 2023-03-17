#include <ableton/Link.hpp>
#include <cassert>
#include <config.hh>
#include <exception>
#include <filesystem>
#include <hello_imgui/hello_imgui.h>
#include <iostream>
#include <MidiFile.h>
#include <optional>
#include <portable-file-dialogs.h>
#include <RtMidi.h>
#include <set>
#include <span>

std::optional<ImGuiKey> to_key(char c) {
	switch (c) {
	case 'A': case 'a': return {ImGuiKey_A};
	case 'B': case 'b': return {ImGuiKey_B};
	case 'C': case 'c': return {ImGuiKey_C};
	case 'D': case 'd': return {ImGuiKey_D};
	case 'E': case 'e': return {ImGuiKey_E};
	case 'F': case 'f': return {ImGuiKey_F};
	case 'G': case 'g': return {ImGuiKey_G};
	case 'H': case 'h': return {ImGuiKey_H};
	case 'I': case 'i': return {ImGuiKey_I};
	case 'J': case 'j': return {ImGuiKey_J};
	case 'K': case 'k': return {ImGuiKey_K};
	case 'L': case 'l': return {ImGuiKey_L};
	case 'M': case 'm': return {ImGuiKey_M};
	case 'N': case 'n': return {ImGuiKey_N};
	case 'O': case 'o': return {ImGuiKey_O};
	case 'P': case 'p': return {ImGuiKey_P};
	case 'Q': case 'q': return {ImGuiKey_Q};
	case 'R': case 'r': return {ImGuiKey_R};
	case 'S': case 's': return {ImGuiKey_S};
	case 'T': case 't': return {ImGuiKey_T};
	case 'U': case 'u': return {ImGuiKey_U};
	case 'V': case 'v': return {ImGuiKey_V};
	case 'W': case 'w': return {ImGuiKey_W};
	case 'X': case 'x': return {ImGuiKey_X};
	case 'Y': case 'y': return {ImGuiKey_Y};
	case 'Z': case 'z': return {ImGuiKey_Z};
	case '0': return {ImGuiKey_0};
	case '1': return {ImGuiKey_1};
	case '2': return {ImGuiKey_2};
	case '3': return {ImGuiKey_3};
	case '4': return {ImGuiKey_4};
	case '5': return {ImGuiKey_5};
	case '6': return {ImGuiKey_6};
	case '7': return {ImGuiKey_7};
	case '8': return {ImGuiKey_8};
	case '9': return {ImGuiKey_9};
	default: return std::nullopt;
	}
}

std::optional<char> from_key(ImGuiKey key) {
	switch (key) {
	case ImGuiKey_A: return {'A'};
	case ImGuiKey_B: return {'B'};
	case ImGuiKey_C: return {'C'};
	case ImGuiKey_D: return {'D'};
	case ImGuiKey_E: return {'E'};
	case ImGuiKey_F: return {'F'};
	case ImGuiKey_G: return {'G'};
	case ImGuiKey_H: return {'H'};
	case ImGuiKey_I: return {'I'};
	case ImGuiKey_J: return {'J'};
	case ImGuiKey_K: return {'K'};
	case ImGuiKey_L: return {'L'};
	case ImGuiKey_M: return {'M'};
	case ImGuiKey_N: return {'N'};
	case ImGuiKey_O: return {'O'};
	case ImGuiKey_P: return {'P'};
	case ImGuiKey_Q: return {'Q'};
	case ImGuiKey_R: return {'R'};
	case ImGuiKey_S: return {'S'};
	case ImGuiKey_T: return {'T'};
	case ImGuiKey_U: return {'U'};
	case ImGuiKey_V: return {'V'};
	case ImGuiKey_W: return {'W'};
	case ImGuiKey_X: return {'X'};
	case ImGuiKey_Y: return {'Y'};
	case ImGuiKey_Z: return {'Z'};
	case ImGuiKey_0: return {'0'};
	case ImGuiKey_1: return {'1'};
	case ImGuiKey_2: return {'2'};
	case ImGuiKey_3: return {'3'};
	case ImGuiKey_4: return {'4'};
	case ImGuiKey_5: return {'5'};
	case ImGuiKey_6: return {'6'};
	case ImGuiKey_7: return {'7'};
	case ImGuiKey_8: return {'8'};
	case ImGuiKey_9: return {'9'};
	default: return std::nullopt;
	}
}

struct Midi_File_State
{
	char key[2] = {};
	int port = 1;
	bool error = false;
};

struct Note_Event
{
	enum class Type
	{
		Note_On,
		Note_Off,
	};

	Type type;
	double time;
	unsigned channel;
	unsigned note;
	unsigned velocity;

	inline static double tempo = -1;

	Note_Event(smf::MidiFile const& file, smf::MidiEvent const& event)
	{
		time = double(event.tick) / file.getTicksPerQuarterNote();

		if (event.isNote()) {
			type = event.isNoteOn() ? Type::Note_On : Type::Note_Off;
			channel = event.getChannel();
			note = event.getKeyNumber();
			velocity = event.getVelocity();
			return;
		}

		if (event.isTempo()) {
			assert(event.getTempoBPM() && "we only support BPM = 120 now");
			return;
		}

		throw std::invalid_argument("Unimplemented midi event type");
	}

	friend std::ostream& operator<<(std::ostream& out, Note_Event const& m)
	{
		switch (m.type) {
		case Note_Event::Type::Note_On:
			return out << "Note On {" << m.channel << ", " << m.note << ", " << m.velocity << ", " << m.time << "}";
		case Note_Event::Type::Note_Off:
			return out << "Note Off {" << m.channel << ", " << m.note << ", " << m.time << "}";
		}
		return out;
	}
};


struct State
{
	State(State const&) = delete;
	State(State const&&) = delete;
	State();
	~State() { the = nullptr; }

	// Render state
	void loop();

	// Read known fields from state cache
	void read_state_cache_from_file();

	// Write current state to cache file
	void write_state_cache_to_file();

	// Do something with state
	void ask_for_midi_files();
	void check_if_midi_files_arrived();
	void refresh_midi_ports_list();

	// Query state
	bool port_number_available(int port);

	// Synchronization mechanism
	void audio_session();

	// Playing mechanism
	void switch_midi_file_to(unsigned id);

	void cleanup_played_notes();

	inline static State *the;

	std::atomic<bool> quit = false;

	std::optional<pfd::open_file> open_file;
	std::set<std::filesystem::path> known_midi_files;
	std::filesystem::path last_open_directory;

	std::vector<Midi_File_State> midi_states;
	std::vector<std::unique_ptr<std::vector<Note_Event>>> events_per_file;
	std::span<Note_Event> events_to_play = {};
	int selected_file = -1;
	// Note played (port, channel, note)
	std::multiset<std::tuple<unsigned, std::uint8_t, std::int8_t>> notes_played{};
	std::mutex play_mutex;

	ableton::Link link;

	// Invariant: has the same order as ports in RTMidi
	std::vector<std::string> midi_ports_list{};
	std::vector<RtMidiOut> midi_output_ports;

	// Synchronization state
	std::atomic<bool> request_start = false;
	std::atomic<bool> request_stop = false;
	std::atomic<bool> is_playing = false;
	std::atomic<double> quantum = 4;
};

State::State()
	: last_open_directory(pfd::path::home())
	, link(120)
{
	the = this;
	link.enable(true);
	link.enableStartStopSync(true);
	refresh_midi_ports_list();
	read_state_cache_from_file();
}

void State::read_state_cache_from_file()
{
	std::ifstream fin(".syncplay");

	if (!fin.is_open()) {
		return;
	}

	std::string last_open_directory;
	if (!std::getline(fin, last_open_directory))
		return;
	this->last_open_directory = last_open_directory;
}

void State::write_state_cache_to_file()
{
	std::ofstream out(".syncplay");
	if (!out.is_open()) {
		return;
	}

	out << last_open_directory.string() << '\n';
}

void State::ask_for_midi_files()
{
	open_file.emplace(
		"Load MIDI files",
		last_open_directory.string(),
		std::vector<std::string> {
				"MIDI (.mid, .midi)", "*.mid *.midi",
				"All files", "*"
		},
		pfd::opt::multiselect
	);
}

std::vector<Note_Event> load_midi_file(std::string const& path);

void State::check_if_midi_files_arrived()
{
	if (open_file && open_file->ready(0)) {
		auto files = open_file->result();
		midi_states.clear();
		events_per_file.clear();
		events_to_play = {};

		last_open_directory = std::filesystem::path(files.back()).parent_path();
		std::move(files.begin(), files.end(), std::inserter(known_midi_files, known_midi_files.begin()));
		midi_states.resize(files.size());
		open_file.reset();
		write_state_cache_to_file();

		{
			auto it = known_midi_files.begin();
			for (auto i = 0u; i < known_midi_files.size(); ++i, ++it) {
				auto const& path = *it;
				events_per_file.push_back(std::make_unique<std::vector<Note_Event>>(load_midi_file(path.string())));

#if 0
				std::cout << *it << '\n';
				for (auto const& events : *events_per_file.back()) {
					std::cout << "  " << events << '\n';
				}
				std::cout << std::endl;
#endif

				std::sort(events_per_file.back()->begin(), events_per_file.back()->end(), [](Note_Event const& lhs, Note_Event const& rhs) {
					return lhs.time < rhs.time;
				});
			}
		}
	}
}

void State::refresh_midi_ports_list()
{
	midi_ports_list.clear();
	midi_output_ports.clear();

	RtMidiOut out;
	for (auto i = 0; i < out.getPortCount(); ++i) {
		midi_ports_list.push_back(out.getPortName(i));
		midi_output_ports.emplace_back().openPort(i);
	}
}

bool State::port_number_available(int port)
{
	return port > 0 && unsigned(port-1) < midi_ports_list.size();
}

template<std::size_t N>
inline void send_message(RtMidiOut &out, std::array<std::uint8_t, N> message)
try {
	out.sendMessage(message.data(), message.size());
} catch (RtMidiError &error) {
	std::cerr << "Failed to use MIDI connection: " << error.getMessage() << std::endl;
	std::exit(33);
}

enum : std::uint8_t
{
	Control_Change = 0b1011'0000,
	Note_Off       = 0b1000'0000,
	Note_On        = 0b1001'0000,
	Program_Change = 0b1100'0000,
};

void send_note_on(RtMidiOut &output, uint8_t channel, int8_t note_number, uint8_t velocity)
{
	send_message(output, std::array { std::uint8_t(Note_On + channel), std::bit_cast<uint8_t>(note_number), velocity });
}

void send_note_off(RtMidiOut &output, uint8_t channel, int8_t note_number)
{
	send_message(output, std::array { std::uint8_t(Note_Off + channel), std::bit_cast<uint8_t>(note_number), uint8_t(0) });
}

void State::audio_session()
{
	bool skip_one = false;
	while (!quit) {
		{
			auto session_state = link.captureAppSessionState();
			auto const time = link.clock().micros();
			auto const beat = session_state.beatAtTime(time, quantum);

			if (request_start) {
				session_state.setIsPlaying(true, time);
				request_start = false;
			}

			if (request_stop) {
				session_state.setIsPlaying(false, time);
				request_stop = false;
			}

			if (!is_playing && session_state.isPlaying()) {
				session_state.requestBeatAtStartPlayingTime(0, quantum);
				if (selected_file >= 0) {
					events_to_play = *events_per_file[selected_file];
				}
				is_playing = true;
			}

			if (is_playing && !session_state.isPlaying()) {
				is_playing = false;
				cleanup_played_notes();
			}

			link.commitAppSessionState(session_state);
		}

		auto session_state = link.captureAppSessionState();
		auto const time = link.clock().micros();
		auto const beat = session_state.beatAtTime(time, quantum);

		if (is_playing && selected_file >= 0) {
			auto to_play = events_to_play;
			while (events_to_play.size() && events_to_play.front().time < beat) {
				auto const& event = events_to_play.front();
				auto port = midi_states[this->selected_file].port;
				assert(port != 0 && "unimplemented");
				if (port < 1 || port > midi_ports_list.size()) {
					events_to_play = events_to_play.subspan(1);
					continue;
				}

				std::lock_guard guard_note_messages{play_mutex};
				switch (event.type) {
				break; case Note_Event::Type::Note_On:
					send_note_on(midi_output_ports[port-1], event.channel, event.note, event.velocity);
					notes_played.emplace(port-1, event.channel, event.note);

				break; case Note_Event::Type::Note_Off:
					send_note_off(midi_output_ports[port-1], event.channel, event.note);
					notes_played.erase(std::tuple{port-1, event.channel, event.note});
				}
				std::cout << event << '\n';
				events_to_play = events_to_play.subspan(1);
			}
			std::this_thread::yield();
			continue;
		}

		// FIXME If we are not playing maybe we should sleep until condition variable is invoked?
		// FIXME If we are playing then we know how much we need to wait for another note to appear
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

void State::switch_midi_file_to(unsigned id)
{
	assert(id < midi_states.size());
	assert(id < events_per_file.size());

	// TODO remove currently playing notes
	events_to_play = *events_per_file[id];
	selected_file = id;
	cleanup_played_notes();
}

void State::cleanup_played_notes()
{
	std::lock_guard cleanup{play_mutex};

	for (auto [port, chan, note] : notes_played) {
		send_note_off(midi_output_ports[port], chan, note);
	}
}

void State::loop()
{
	auto session_state = link.captureAppSessionState();
	auto const time = link.clock().micros();
	double bpm = session_state.tempo();
	bool changed_session_state = false;

	double const quantum = 4;
	auto const beats = session_state.beatAtTime(time, quantum);

	check_if_midi_files_arrived();

	ImGui::SeparatorText("Shared state settings"); {
		if (ImGui::InputDouble("BPM", &bpm, 1, 10, "%.0f")) {
			session_state.setTempo(bpm, time);
			changed_session_state = true;
		}

		ImGui::Text("Peers: %zu", link.numPeers());
		ImGui::Text("Beats: %.0f", std::floor(beats));
		ImGui::Text("Playing: %s", is_playing ? "Playing" : "Stopped");
	}

	ImGui::SeparatorText("MIDI ports"); {
		// TODO Virtual port
		if (ImGui::Button("Refresh ports list")) {
			refresh_midi_ports_list();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Updates lists of available MIDI ports");
		}

		unsigned i = 0;
		for (auto const& port_name : midi_ports_list) {
			ImGui::Text("%u. %s", ++i, port_name.c_str());
		}
	}

	ImGui::SeparatorText("Loaded MIDI files"); {
		if (ImGui::Button("Add MIDI file")) {
			ask_for_midi_files();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Loads new MIDI file that will be available for synchronized playing");
		}

		if (not known_midi_files.empty()) {
			if (ImGui::BeginTable("known_midi_files", 3)) {
				/* header */ {
					static char const* columns[] = {
						"Key",
						"Port",
						"File",
					};
					unsigned i = 0;
					ImGui::TableNextRow();
					for (auto const& header : columns) {
						ImGui::TableSetColumnIndex(i++);
						ImGui::Text("%s", header);
					}
				}

				auto it = known_midi_files.begin();
				for (auto i = 0u; i < known_midi_files.size(); ++i, ++it) {
					auto const& file = *it;
					auto& file_state = midi_states[i];
					ImGui::TableNextRow();

					if (i == selected_file) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.65f)));
					}

					static std::string key_label = "##key" + std::to_string(i);
					ImGui::TableSetColumnIndex(0);
					ImGui::InputText(key_label.c_str(), file_state.key, sizeof(file_state.key));

					static std::string port_label = "##port" + std::to_string(i);
					ImGui::TableSetColumnIndex(1);
					bool valid_port = port_number_available(file_state.port);
					if (!valid_port) {
						ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
					}
					ImGui::InputInt(port_label.c_str(), &file_state.port, 1, 1);
					if (!valid_port) {
						ImGui::PopStyleColor();
					}

					static std::string path = file.string();
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%s", path.c_str());
				}
				ImGui::EndTable();
			}
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
		if (is_playing) {
			request_stop = true;
		} else {
			request_start = true;
		}
	}

	for (auto i = 0u; i < midi_states.size(); ++i) {
		auto const& midi_state = midi_states[i];
		if (*midi_state.key != '\0') {
			if (auto key = to_key(*midi_state.key)) {
				if (ImGui::IsKeyPressed(*key)) {
					switch_midi_file_to(i);
				}
			}
		}
	}
}

std::vector<Note_Event> load_midi_file(std::string const& path)
{
	smf::MidiFile midi(path);
	// std::cout << "File duration: " << midi.getFileDurationInSeconds() << std::endl;

	assert(midi.status());

	midi.doTimeAnalysis();
	midi.linkEventPairs();
	midi.linkNotePairs();
	midi.sortTracks();
	midi.joinTracks();
	midi.absoluteTicks();

	auto ticks = midi.getTicksPerQuarterNote();

	assert(midi.getTrackCount() == 1);

	std::vector<Note_Event> events;

	auto const& track = midi[0];
	for (auto j = 0u; j < track.getEventCount(); ++j) {
		auto const& event = track.getEvent(j);
		try { events.emplace_back(midi, event); } catch (std::exception const&) {}
	}

	return events;
}

int main()
{
	assert(pfd::settings::available());
	pfd::settings::verbose(false);

	State s;

	std::thread th(&State::audio_session, std::ref(s));

	HelloImGui::Run([]{ State::the->loop(); }, "SyncPlay " + std::string(Version), false, true, {600,600});

	s.quit = true;
	th.join();

	return 0;
}
