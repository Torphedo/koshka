#include "boolset.hxx"
#include "bitmanip.h"
#include <common/logging.h>
#include <cstdlib>
#include <cstring>

boolset::boolset(u32 bit_count) {
    // Round up a byte if the count isn't a multiple of 8
    byte_size = (bit_count / 8) + (bit_count % 8 != 0);
    buf = (u8*)calloc(1, byte_size);
    initialized = (buf != nullptr);
}

boolset::~boolset() {
    free(buf); // free(NULL) is safe! Don't bother me about it! - torph
}

bool boolset::get_bit(u32 pos) {
    // TODO: Factor out common byte/bit pos code? Is it even worth it?
    const u32 byte_pos = pos / 8;
    // Bit pos of 0 means the MSB, which is considered bit 7 by helper functions
    // And likewise, bit pos 7 == LSB, considered bit 0 by helper functions.
    const u32 bit_pos = 7 - (pos % 8);

    return GET_SINGLE_BIT(buf[byte_pos], bit_pos);
}

void boolset::set_bit(u32 pos, bool value) {
    const u32 byte_pos = pos / 8;
    const u32 bit_pos = 7 - (pos % 8);

    // Load byte and apply the new value
    u8 byte = buf[byte_pos];
    byte |= ((u8)1) << bit_pos;

    // Save edited byte back to buffer
    buf[byte_pos] = byte;
}

/// @brief Set all bits below a certain bit within a byte.
/// @param byte The byte to edit
/// @param bits_from_msb The bit within the byte to start setting from. MSB == 0, LSB == 7
/// @param value The value to set bits in the range to
void set_in_byte(u8* byte, u8 bits_from_msb, bool value) {
// TODO:           ^ Change input byte from pointer to reference?
    if (bits_from_msb > 7) {
        LOG_MSG(warning, "Programmer error: out of bounds size %d (ignored)\n", bits_from_msb);
        return;
    }

    const u8 size_to_set = 8 - bits_from_msb;
    if (value) {
        // Create a bitmask with the right number of bits and set them all
        *byte |= (u8)MAX_VAL_FOR_SIZE(size_to_set);
    } else {
        // Same as above, but we invert the mask and use & to clear them
        *byte &= ~((u8)MAX_VAL_FOR_SIZE(size_to_set));
    }
}

void boolset::set_range(u32 pos, u32 size, bool value) {
    const u32 start_byte_pos = pos / 8;
    const u8 start_bit_pos = (pos % 8);

    // The last byte position this operation will touch. That last part is
    // rounding up a byte when it's not a multiple of 8.
    const u8 end_pos = start_byte_pos + (size / 8) + (size % 8 != 0);

    if (start_byte_pos >= byte_size) {
        LOG_MSG(warning, "Programmer error: out of bounds start position %d (ignored)\n", pos);
        return;
    }

    // These checks could be merged, but this makes more useful error messages.
    if (end_pos >= byte_size) {
        LOG_MSG(warning, "Programmer error: Operation w/ bit position %d and bit size %d exceeds %d-byte capacity. (ignored)\n", pos, size, byte_size);
        return;
    }

    // Handle setting bits at the start that aren't on a byte boundary
    if (start_bit_pos > 0) {
        set_in_byte(&buf[start_byte_pos], start_bit_pos, value);
    }

    // memset() as much as possible at once
    const u8 byte_val = value ? 0xFF : 0x00;
    memset(&buf[start_byte_pos + 1], byte_val, size / 8);

    // Handle setting bits at the end that trail off the byte boundary
    if (size % 8 != 0) {
        set_in_byte(&buf[size / 8], size % 8, value);
    }
}


void boolset::set_all(bool value) {
    const u8 byte_val = value ? 0xFF : 0x00;

    memset(buf, byte_val, byte_size);
}
