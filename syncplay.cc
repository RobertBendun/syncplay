#include <MidiFile.h>
#include <ableton/Link.hpp>
#include <cassert>
#include <config.hh>
#include <filesystem>
#include <hello_imgui/hello_imgui.h>
#include <iostream>
#include <optional>
#include <portable-file-dialogs.h>
#include <set>

struct State
{
	State(State const&) = delete;
	State(State const&&) = delete;
	State();
	~State() { the = nullptr; }

	void loop();
	void ask_for_midi_files();
	void check_if_midi_files_arrived();

	inline static State *the;

	std::optional<pfd::open_file> open_file;
	std::set<std::filesystem::path> known_midi_files;
	std::filesystem::path last_open_directory;

	ableton::Link link;
};

State::State()
	: last_open_directory(pfd::path::home())
	, link(120)
{
	the = this;
	link.enable(true);
	link.enableStartStopSync(true);
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
		std::move(files.begin(), files.end(), std::inserter(known_midi_files, known_midi_files.begin()));
		open_file.reset();
	}
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

	ImGui::SeparatorText("Personal state settings"); {
		if (ImGui::Button("Add MIDI file")) {
			ask_for_midi_files();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Loads new MIDI file that will be available for synchronized playing");
		}
	}

	ImGui::SeparatorText("Loaded MIDI files"); {
		if (not known_midi_files.empty()) {
			if (ImGui::BeginTable("known_midi_files", 3)) {
				static char const* columns[] = {
					"Key",
					"Port",
					"File",
				};

				/* header */ {
					unsigned i = 0;
					ImGui::TableNextRow();
					for (auto const& header : columns) {
						ImGui::TableSetColumnIndex(i++);
						ImGui::Text("%s", header);
					}
				}

				for (auto const& file : known_midi_files) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(1);
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
