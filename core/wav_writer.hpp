// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Writes 32-bit float WAV files (format 3, IEEE float), mono or with several channels of the same length.
#pragma once

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace grmh
{
class WavWriter
{
public:
    static void write(const std::string& path, const std::vector<float>& samples, int sample_rate)
    {
        write(path, std::vector<std::vector<float>>{samples}, sample_rate);
    }

    // the channels interleaved, the first left
    static void write(const std::string& path, const std::vector<std::vector<float>>& channels, int sample_rate)
    {
        if (channels.empty())
            throw std::invalid_argument("a WAV file needs a channel");
        const size_t frames = channels.front().size();
        for (const auto& c : channels)
            if (c.size() != frames)
                throw std::invalid_argument("the channels of a WAV file have one length");
        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot write " + path);
        const auto count = static_cast<std::uint16_t>(channels.size());
        const auto data_bytes = static_cast<std::uint32_t>(frames * count * sizeof(float));
        file.write("RIFF", 4);
        put(file, static_cast<std::uint32_t>(36 + data_bytes));
        file.write("WAVEfmt ", 8);
        put(file, std::uint32_t{16});                                                 // fmt chunk size
        put(file, std::uint16_t{3});                                                  // IEEE float
        put(file, count);                                                             // channels
        put(file, static_cast<std::uint32_t>(sample_rate));
        put(file, static_cast<std::uint32_t>(sample_rate * count * sizeof(float)));  // bytes per second
        put(file, static_cast<std::uint16_t>(count * sizeof(float)));                // block align
        put(file, std::uint16_t{32});                                                 // bits per sample
        file.write("data", 4);
        put(file, data_bytes);
        for (size_t n = 0; n < frames; ++n)
            for (const auto& c : channels)
                put(file, c[n]);
    }

private:
    template <class T>
    static void put(std::ofstream& file, T value) // little endian, as every platform the plugin targets
    {
        file.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }
};
} // namespace grmh
