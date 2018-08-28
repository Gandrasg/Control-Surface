#pragma once

#include "MIDIInputElementNote.hpp"

/** An abstract base class for MIDI note input. */
class MIDINote_Base : public MIDIInputElementNote {
  protected:
    MIDINote_Base(); // unused, only for virtual inheritance
    MIDINote_Base(uint8_t note, uint8_t channel)
        : MIDIInputElementNote(channel, note) {}

  public:
    virtual uint8_t getValue() const = 0;
    bool getState() const { return getValue() >= NOTE_VELOCITY_THRESHOLD; }

  protected:
    uint8_t getValueFromMIDIMessage(const MIDI_message_matcher &midimsg) {
        return midimsg.type == NOTE_ON ? midimsg.data2 : 0;
    }

  private:
    virtual bool updateImpl(const MIDI_message_matcher &midimsg) = 0;
};

// -------------------------------------------------------------------------- //

/** A class for MIDI note input. */
class MIDINote : virtual public MIDINote_Base {
  public:
    MIDINote(uint8_t note, uint8_t channel = 1)
        : MIDINote_Base(note, channel) {}

    uint8_t getValue() const override { return value; }

    void reset() override { value = 0; }

  private:
    bool updateImpl(const MIDI_message_matcher &midimsg) override {
        this->value = getValueFromMIDIMessage(midimsg);
        return true;
    }

    uint8_t value = 0;
};

// -------------------------------------------------------------------------- //

#include <Banks/BankableMIDIInputAddressable.hpp>

namespace Bankable {

/** A class for Bankable MIDI note input. */
template <uint8_t N>
class MIDINote : virtual public MIDINote_Base,
                 public BankableMIDIInputAddressable<N> {
  public:
    MIDINote(const BankConfigAddressable<N> &config, uint8_t note,
             uint8_t channel = 1)
        : MIDINote_Base(note, channel), BankableMIDIInputAddressable<N>(
                                            config) {}

    uint8_t getValue() const override {
        return values[this->getSelection()]; // TODO: N
    }

    void reset() override {
        for (uint8_t &value : values)
            value = 0;
    }

  private:
    bool updateImpl(const MIDI_message_matcher &midimsg) override {
        uint8_t index = this->getIndex(midimsg.channel, midimsg.data1,
                                       getBaseChannel(), getBaseAddress());
        uint8_t value = getValueFromMIDIMessage(midimsg);
        this->values[index] = value;
        return true;
    }

    bool matchAddress(uint8_t targetAddress) const override {
        return BankableMIDIInputAddressable<N>::matchAddress(targetAddress,
                                                             getBaseAddress());
    }

    bool matchChannel(uint8_t targetChannel) const override {
        return BankableMIDIInputAddressable<N>::matchChannel(targetChannel,
                                                             getBaseChannel());
    }

    void onBankSettingChange() const override { display(); }

    uint8_t values[N] = {};
};

} // namespace Bankable