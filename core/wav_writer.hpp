// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Writes mono 32-bit float WAV files (format 3, IEEE float).
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
        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot write " + path);
        const auto data_bytes = static_cast<std::uint32_t>(samples.size() * sizeof(float));
        file.write("RIFF", 4);
        put(file, static_cast<std::uint32_t>(36 + data_bytes));
        file.write("WAVEfmt ", 8);
        put(file, std::uint32_t{16});                                   // fmt chunk size
        put(file, std::uint16_t{3});                                    // IEEE float
        put(file, std::uint16_t{1});                                    // mono
        put(file, static_cast<std::uint32_t>(sample_rate));
        put(file, static_cast<std::uint32_t>(sample_rate * sizeof(float))); // bytes per second
        put(file, static_cast<std::uint16_t>(sizeof(float)));           // block align
        put(file, std::uint16_t{32});                                   // bits per sample
        file.write("data", 4);
        put(file, data_bytes);
        file.write(reinterpret_cast<const char*>(samples.data()), data_bytes);
    }

private:
    template <class T>
    static void put(std::ofstream& file, T value) // little endian, as every platform the plugin targets
    {
        file.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }
};
} // namespace grmh
