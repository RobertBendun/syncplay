#include <ableton/Link.hpp>
#include <cassert>
#include <config.hh>
#include <filesystem>
#include <hello_imgui/hello_imgui.h>
#include <iostream>
#include <MidiFile.h>
#include <optional>
#include <portable-file-dialogs.h>
#include <RtMidi.h>
#include <set>

struct Midi_File_State
{
	char key[2];
	int port = 1;
};

struct State
{
	State(State const&) = delete;
	State(State const&&) = delete;
	State();
	~State() { the = nullptr; }

	// Render state
	void loop();

	// Do something with state
	void ask_for_midi_files();
	void check_if_midi_files_arrived();
	void refresh_midi_ports_list();

	// Query state
	bool port_number_available(int port);

	inline static State *the;

	std::optional<pfd::open_file> open_file;
	std::set<std::filesystem::path> known_midi_files;
	std::filesystem::path last_open_directory;

	std::vector<Midi_File_State> midi_states;

	ableton::Link link;

	// Invariant: has the same order as ports in RTMidi
	std::vector<std::string> midi_ports_list;
};

State::State()
	: last_open_directory(pfd::path::home())
	, link(120)
{
	the = this;
	link.enable(true);
	link.enableStartStopSync(true);
	refresh_midi_ports_list();
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

void State::check_if_midi_files_arrived()
{
	if (open_file && open_file->ready(0)) {
		auto files = open_file->result();
		midi_states.clear();
		std::move(files.begin(), files.end(), std::inserter(known_midi_files, known_midi_files.begin()));
		midi_states.resize(files.size());
		open_file.reset();
	}
}

void State::refresh_midi_ports_list()
{
	midi_ports_list.clear();
	RtMidiOut out;
	for (auto i = 0; i < out.getPortCount(); ++i) {
		midi_ports_list.push_back(out.getPortName(i));
	}
}

bool State::port_number_available(int port)
{
	return port > 0 && unsigned(port-1) < midi_ports_list.size();
}

void State::loop()
{
	auto session_state = link.captureAppSessionState();
	auto const time = link.clock().micros();
	double bpm = session_state.tempo();
	bool changed_session_state = false;

	check_if_midi_files_arrived();

	ImGui::SeparatorText("Shared state settings"); {
		if (ImGui::InputDouble("BPM", &bpm, 1, 10, "%.0f")) {
			session_state.setTempo(bpm, time);
			changed_session_state = true;
		}

		ImGui::Text("Peers: %zu", link.numPeers());
		ImGui::Text("Playing: %s", session_state.isPlaying() ? "Playing" : "Stopped");
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

				unsigned i = 0;
				for (auto const& file : known_midi_files) {
					auto &file_state = midi_states[i++];
					ImGui::TableNextRow();

					std::string label = "##key" + std::to_string(i);
					ImGui::TableSetColumnIndex(0);
					ImGui::InputText(label.c_str(), file_state.key, sizeof(file_state.key));

					label = "##port" + std::to_string(i);
					ImGui::TableSetColumnIndex(1);
					bool valid_port = port_number_available(file_state.port);
					if (!valid_port) {
						ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
					}
					ImGui::InputInt(label.c_str(), &file_state.port, 1, 1);
					if (!valid_port) {
						ImGui::PopStyleColor();
					}

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%s", file.c_str());
				}
				ImGui::EndTable();
			}
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
		session_state.setIsPlaying(not session_state.isPlaying(), time);
		changed_session_state = true;
	}

	if (changed_session_state) {
		link.commitAppSessionState(session_state);
	}
}

static double tempo = -1;

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

	Note_Event(smf::MidiFile const& file, smf::MidiEvent const& event)
	{
		time = event.tick * 60.0 / tempo / file.getTicksPerQuarterNote();

		if (event.isNote()) {
			assert(tempo == -1);
			type = event.isNoteOn() ? Type::Note_On : Type::Note_Off;
			channel = event.getChannel();
			note = event.getKeyNumber();
			velocity = event.getVelocity();
			return;
		}

		if (event.isTempo()) {
			double microseconds = event.getTempoMicroseconds();
			tempo = 60.0 / microseconds * 1000000.0;
			return;
		}
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

std::vector<Note_Event> load_midi_file(std::string const& path)
{
	smf::MidiFile midi("/home/diana/uam/project/y/wk/A/A rytm.mid");
	// std::cout << "File duration: " << midi.getFileDurationInSeconds() << std::endl;

	assert(midi.getStatus());

	midi.doTimeAnalysis();
	midi.linkEventPairs();
	midi.linkNotePairs();
	midi.sortTracks();
	midi.joinTracks();
	midi.absoluteTicks();
	// midi.deltaTicks();

	auto ticks = midi.getTicksPerQuarterNote();

	assert(midi.getTrackCount() == 1);

	std::vector<Note_Event> events;

	auto const& track = midi[0];
	for (auto j = 0u; j < track.getEventCount(); ++j) {
		auto const& event = track.getEvent(j);
		events.emplace_back(midi, event);
	}

	return events;
}

int main()
{
	assert(pfd::settings::available());
	pfd::settings::verbose(false);

	State s;
	HelloImGui::Run([]{ State::the->loop(); }, "SyncPlay " + std::string(Version), false, true, {600,600});

	return 0;
}
