// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The build is sound: cppgrm is fetched, a minimal network is solved and gives back what its relation says.
#include <cmath>

#include "grm.hpp"
#include "support/test_report.hpp"

int main()
{
    grmh::test::TestReport report("scaffold");

    // two quantities tied by one exact relation, y = 2 x, and x measured: the network must give y
    grm::Network net;
    net.add_node("x", 1.0);
    net.add_node("y", 1.0);
    net.add_log("double", {{"y", 1}, {"x", -1}}, std::log10(2.0), 1e-9);
    const auto r = net.solve({{"x", {3.0, 1e-9}}});

    report.expect(r.converged, "the network converges");
    report.expect_near(r["x"], 3.0, 1e-9, "the measured node keeps its value");
    report.expect_near(r["y"], 6.0, 1e-9, "the relation gives the other node");
    return report.finish();
}
