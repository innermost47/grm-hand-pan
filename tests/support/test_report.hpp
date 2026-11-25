// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The checks shared by every test: each one prints a line, the report gives the exit code ctest reads.
#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace grmh::test
{
class TestReport
{
public:
    explicit TestReport(std::string suite) : suite_(std::move(suite)) {}

    // a condition that must hold
    bool expect(bool condition, const std::string& what)
    {
        ++checks_;
        if (!condition)
            ++failures_;
        std::printf("[%s] %s: %s\n", condition ? "ok" : "FAIL", suite_.c_str(), what.c_str());
        return condition;
    }

    // a value within a relative tolerance of its reference (a published measurement and its spread)
    bool expect_near(double value, double reference, double relative_tolerance, const std::string& what)
    {
        const bool finite = std::isfinite(value) && std::isfinite(reference);
        const double error = finite ? std::fabs(value - reference) / std::fabs(reference) : INFINITY;
        char detail[160];
        std::snprintf(detail, sizeof detail, " (%.6g, expected %.6g within %.3g %%)", value, reference,
                      100.0 * relative_tolerance);
        return expect(finite && error <= relative_tolerance, what + detail);
    }

    // the exit code of the test program
    int finish() const
    {
        std::printf("%s: %d/%d checks passed\n", suite_.c_str(), checks_ - failures_, checks_);
        return failures_ == 0 && checks_ > 0 ? 0 : 1;
    }

private:
    std::string suite_;
    int checks_ = 0;
    int failures_ = 0;
};
} // namespace grmh::test
