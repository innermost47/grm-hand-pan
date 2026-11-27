// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin: a D Kurd handpan played from MIDI and from its own roll. Keys play notes (the MIDI mode by default):
// each note-on strikes the field tuned to it, at its velocity, to the sample; note-offs are ignored (a handpan rings
// until it fades); the pattern being edited plays along the song (the host's clock, or the plugin's own), its strikes
// to the sample too. Keys play patterns: from C1, a key held plays its pattern from the next bar line, let go stops it.
// Song on: the arranger's song plays instead, once, from the song's start. The sound model comes from the
// solver thread, the compiled pattern from the interface, both through lock-free mailboxes; the audio thread never
// allocates, locks or solves.
#pragma once

#include "../core/arrangement_compiler.hpp"
#include "../core/block_strikes.hpp"
#include "../core/engine.hpp"
#include "../core/mailbox.hpp"
#include "../core/pattern_library.hpp"
#include "../core/pattern_player.hpp"
#include "../core/pattern_sequencer.hpp"
#include "../core/transport.hpp"
#include "JuceHeader.h"
#include "SolverThread.h"

class HandPanProcessor : public juce::AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
	HandPanProcessor();
	~HandPanProcessor() override;

	void prepareToPlay(double sample_rate, int block_size) override;
	void releaseResources() override {}
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override { return true; }

	const juce::String getName() const override { return JucePlugin_Name; }
	bool acceptsMidi() const override { return true; }
	bool producesMidi() const override { return false; }
	bool isMidiEffect() const override { return false; }
	double getTailLengthSeconds() const override;

	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	void setCurrentProgram(int) override {}
	const juce::String getProgramName(int) override { return {}; }
	void changeProgramName(int, const juce::String&) override {}

	void getStateInformation(juce::MemoryBlock& data) override;
	void setStateInformation(const void* data, int size) override;

	// true once the instrument is solved and playing (for hosts and tests)
	bool instrument_ready() const { return solver_.ready(); }
	// true while the instrument is being solved (the interface shows it)
	bool solving() const { return solver_.solving(); }

	juce::AudioProcessorValueTreeState& parameters() { return parameters_; }
	grmh::Transport& transport() { return transport_; }
	bool standalone() const { return wrapperType == wrapperType_Standalone; }
	// the tempo of the last block (the host's, or the plugin's own)
	double shown_tempo() const { return shown_tempo_.load(); }

	// message thread: the pattern being edited (and played), and a new one (compiled here, played from the next
	// block); the version grows at every change of the patterns, for the views
	const grmh::Pattern& pattern() const { return library_.pattern(); }
	void set_pattern(const grmh::Pattern& pattern);
	int pattern_version() const { return pattern_version_; }
	// message thread: every pattern of the banks; another one edited and played; a slot changed (pasted, renamed,
	// cleared)
	const grmh::PatternLibrary& library() const { return library_; }
	void select_pattern(grmh::SlotId id);
	void set_slot(grmh::SlotId id, const grmh::PatternSlot& slot);
	// message thread: the player's hands (which one is strong), the pattern played again with them
	const grmh::PlayerHands& hands() const { return hands_; }
	void set_hands(const grmh::PlayerHands& hands);
	// the instrument's notes, the ding first
	const std::vector<int>& notes() const { return notes_; }
	// message thread: a note heard now (a hit just drawn), struck at the next block's start
	void audition(int note, int velocity);
	// the pattern a MIDI key plays (bank * PATTERNS_PER_BANK + slot), -1: none (for the views)
	int shown_pattern() const { return shown_pattern_.load(); }

	// message thread: the song (the arranger's blocks), and a new one; whether the song plays (the "Song" switch);
	// where the blocks lie in the song; the song's beat where the pattern being edited started, for the roll's
	// playhead (0 out of the song)
	const grmh::Arrangement& arrangement() const { return arrangement_; }
	void set_arrangement(const grmh::Arrangement& arrangement);
	bool song_mode() const;
	bool follow_playhead() const;
	std::vector<grmh::BlockSpan> song_spans() const;
	double pattern_origin(double beat) const;

	// message thread, to render offline: the instrument as last solved (null before), the pattern being edited and
	// the song compiled, the tempo they play at (the host's last one, or the plugin's own), the volume's gain
	std::shared_ptr<const grmh::SoundModel> sound_model() const { return solver_.shown_model(); }
	grmh::CompiledPattern compiled_pattern() const { return compile(library_.pattern()); }
	grmh::CompiledPattern compiled_song() const;
	double played_tempo() const;
	float output_gain() const;
	grmh::SlotId active_song_slot(double beat) const;

private:
	static juce::AudioProcessorValueTreeState::ParameterLayout create_layout();
	grmh::HostPosition host_position() const;
	void render(juce::AudioBuffer<float>& buffer, int start, int count);
	// the pattern's strikes before sample `until`, each after rendering up to it; `position` follows
	void play_pattern_until(juce::AudioBuffer<float>& buffer, int& position, int& next, int until);

	juce::AudioProcessorValueTreeState parameters_;
	grmh::Mailbox<grmh::SoundModel> models_; // declared before the solver: outlives it
	SolverThread solver_;
	grmh::Engine engine_;
	const grmh::SoundModel* playing_ = nullptr;

	void compile_library(); // message thread: the patterns, the song and the hands to the audio thread
	grmh::CompiledPattern compile(const grmh::Pattern& pattern) const;

	std::vector<int> notes_;
	grmh::PatternLibrary library_;                   // message thread
	grmh::Arrangement arrangement_;                  // message thread
	grmh::PlayerHands hands_;                        // message thread
	int pattern_version_ = 0;                        // message thread
	grmh::Mailbox<grmh::CompiledLibrary> libraries_; // message thread -> audio thread
	const grmh::CompiledLibrary* playing_library_ = nullptr;
	grmh::PatternPlayer player_; // the patterns the keys ask for
	grmh::BlockStrikes block_strikes_;
	std::atomic<int> shown_pattern_{ -1 };
	bool patterns_mode() const;
	void read_pattern_keys(const juce::MidiBuffer& midi);
	void gather_strikes(const grmh::ClockBlock& clock, int samples, bool song, bool keys_play_patterns);
	grmh::Transport transport_;
	std::atomic<double> shown_tempo_{ 0.0 };
	double sample_rate_ = 0.0;
	juce::SmoothedValue<float> gain_{ 1.0f }; // the volume, gliding

	// the notes heard from the interface: a lock-free queue to the audio thread
	static constexpr int AUDITION_QUEUE = 32;
	juce::AbstractFifo auditions_{ AUDITION_QUEUE };
	std::array<grmh::BlockStrike, AUDITION_QUEUE> audition_strikes_{};
	void play_auditions();
	void parameterChanged(const juce::String& parameter_id, float new_value);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HandPanProcessor)
};
