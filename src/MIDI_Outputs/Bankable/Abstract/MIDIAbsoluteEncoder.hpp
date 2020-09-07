#pragma once

#include <AH/STL/type_traits> // std::make_signed
#include <AH/STL/utility>     // std::forward
#include <Banks/BankableAddresses.hpp>
#include <Def/Def.hpp>
#include <Def/TypeTraits.hpp>
#include <MIDI_Outputs/Abstract/MIDIOutputElement.hpp>

#ifdef ARDUINO
#include <Submodules/Encoder/Encoder.h>
#else 
#include <Encoder.h> // Mock
#endif

AH_DIAGNOSTIC_WERROR()

BEGIN_CS_NAMESPACE

namespace Bankable {

/**
 * @brief   An abstract class for rotary encoders that send absolute MIDI 
 *          events.
 */
template <class Enc, uint8_t NumBanks, class BankAddress, class Sender>
class GenericMIDIAbsoluteEncoder : public MIDIOutputElement {
  public:
    GenericMIDIAbsoluteEncoder(const BankAddress &address, Enc &&encoder,
                               int16_t multiplier, uint8_t pulsesPerStep,
                               const Sender &sender)
        : encoder(std::forward<Enc>(encoder)), address(address),
          multiplier(multiplier), pulsesPerStep(pulsesPerStep), sender(sender) {
    }

    void begin() override { begin_if_possible(encoder); }

    void update() override {
        Enc_t encval = encoder.read();
        // If Enc_t is an unsigned type, integer overflow is well-defined, which
        // is what we want when Enc_t is small and expected to overflow.
        // However, we need it to be signed because we're interested  in the
        // delta.
        int16_t delta = SignedEnc_t(Enc_t(encval - deltaOffset));
        delta = delta * multiplier / pulsesPerStep;
        if (delta) {
            address.lock();
            int16_t oldValue = values[address.getSelection()];
            int16_t newValue = oldValue + delta;
            newValue = constrain(newValue, 0, maxValue);
            if (oldValue != newValue) {
                sender.send(newValue, address.getActiveAddress());
                values[address.getSelection()] = newValue;
            }
            address.unlock();
            deltaOffset += Enc_t(delta * pulsesPerStep / multiplier);
        }
    }

    /**
     * @brief   Get the absolute value of the encoder in the given bank.
     */
    uint16_t getValue(setting_t bank) const { return values[bank]; }
    /**
     * @brief   Get the absolute value of the encoder in the active bank.
     */
    uint16_t getValue() const { return getValue(address.getSelection()); }

    /**
     * @brief   Set the absolute value of the encoder in the given bank.
     */
    void setValue(uint16_t value, setting_t bank) { values[bank] = value; }
    /**
     * @brief   Set the absolute value of the encoder in the active bank.
     */
    void setValue(uint16_t value) { setValue(value, address.getSelection()); }

  private:
    Enc encoder;
    BankAddress address;
    int16_t multiplier;
    uint8_t pulsesPerStep;
    Array<uint16_t, NumBanks> values = {{}};
    using Enc_t = decltype(encoder.read());
    using SignedEnc_t = typename std::make_signed<Enc_t>::type;
    Enc_t deltaOffset = 0;

    constexpr static int16_t maxValue = uint16_t(1u << Sender::precision()) - 1;

  public:
    Sender sender;
};

#if defined(Encoder_h_) || not defined(ARDUINO)

template <uint8_t NumBanks, class BankAddress, class Sender>
using MIDIAbsoluteEncoder =
    GenericMIDIAbsoluteEncoder<Encoder, NumBanks, BankAddress, Sender>;

template <uint8_t NumBanks, class BankAddress, class Sender>
using BorrowedMIDIAbsoluteEncoder =
    GenericMIDIAbsoluteEncoder<Encoder &, NumBanks, BankAddress, Sender>;

#endif

} // namespace Bankable

END_CS_NAMESPACE

AH_DIAGNOSTIC_POP()
