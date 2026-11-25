// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Hands a finished object from the solver thread to the audio thread without a lock and without freeing memory on
// the audio thread (the pattern of grm-guitare's grmg::Mailbox).
#pragma once

#include <atomic>

namespace grmh
{
template <class T>
class Mailbox
{
public:
    Mailbox() = default;
    Mailbox(const Mailbox&) = delete;
    Mailbox& operator=(const Mailbox&) = delete;
    ~Mailbox()
    {
        delete pending_.load();
        delete trash_.load();
        delete current_;
    }

    // solver thread: offers a new object, replacing one the audio thread has not taken yet
    void post(T* object)
    {
        collect();
        delete pending_.exchange(object);
    }

    // solver thread: frees what the audio thread has put aside
    void collect() { delete trash_.exchange(nullptr); }

    // audio thread: the newest object; the one it replaces goes to the trash, freed later by the solver
    const T* take()
    {
        if (trash_.load() == nullptr)
            if (T* fresh = pending_.exchange(nullptr))
            {
                trash_.store(current_);
                current_ = fresh;
            }
        return current_;
    }

private:
    std::atomic<T*> pending_{nullptr};
    std::atomic<T*> trash_{nullptr};
    T* current_ = nullptr;
};
} // namespace grmh
