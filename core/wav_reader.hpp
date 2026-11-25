// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Reads WAV files (16-bit PCM or 32-bit float, any channel count: the first channel is kept), for the tools that
// measure recordings.
#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace grmh
{
struct WavData
{
    std::vector<float> samples; // the first channel
    int sample_rate = 0;
};

class WavReader
{
public:
    static WavData read(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot read " + path);
        char tag[4];
        file.read(tag, 4);
        if (std::strncmp(tag, "RIFF", 4) != 0)
            throw std::runtime_error(path + " is not a RIFF file");
        get<std::uint32_t>(file);
        file.read(tag, 4);
        if (std::strncmp(tag, "WAVE", 4) != 0)
            throw std::runtime_error(path + " is not a WAVE file");

        std::uint16_t format = 0, channels = 0, bits = 0;
        WavData data;
        while (file.read(tag, 4))
        {
            const auto size = get<std::uint32_t>(file);
            if (std::strncmp(tag, "fmt ", 4) == 0)
            {
                format = get<std::uint16_t>(file);
                channels = get<std::uint16_t>(file);
                data.sample_rate = static_cast<int>(get<std::uint32_t>(file));
                get<std::uint32_t>(file); // bytes per second
                get<std::uint16_t>(file); // block align
                bits = get<std::uint16_t>(file);
                file.seekg(size - 16, std::ios::cur);
            }
            else if (std::strncmp(tag, "data", 4) == 0)
            {
                const bool is_float = format == 3 && bits == 32, is_pcm16 = format == 1 && bits == 16;
                if (!is_float && !is_pcm16 || channels == 0)
                    throw std::runtime_error(path + ": only 16-bit PCM or 32-bit float WAV files are read");
                const size_t frames = size / (bits / 8) / channels;
                for (size_t i = 0; i < frames; ++i)
                    for (std::uint16_t c = 0; c < channels; ++c)
                    {
                        const float s = is_float ? get<float>(file) : get<std::int16_t>(file) / 32768.0f;
                        if (c == 0)
                            data.samples.push_back(s);
                    }
                return data;
            }
            else
                file.seekg(size + (size & 1u), std::ios::cur); // chunks are word-aligned
        }
        throw std::runtime_error(path + " has no data chunk");
    }

private:
    template <class T>
    static T get(std::ifstream& file)
    {
        T value{};
        file.read(reinterpret_cast<char*>(&value), sizeof(T));
        return value;
    }
};
} // namespace grmh
