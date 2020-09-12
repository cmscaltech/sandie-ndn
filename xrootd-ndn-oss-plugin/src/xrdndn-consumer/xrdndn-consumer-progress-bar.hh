// SANDIE: National Science Foundation Award #1659403
// Copyright (c) 2018-2020 California Institute of Technology
//
// Author: Catalin Iordache <catalin.iordache@cern.ch>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef XRDNDN_CONSUMER_PROGRESS_BAR_HH
#define XRDNDN_CONSUMER_PROGRESS_BAR_HH

#include <chrono>
#include <iostream>

namespace xrdndnconsumer {
class ProgressBar {
  private:
    const uint64_t m_total;
    uint64_t m_current = 0;
    const uint16_t m_width;
    bool m_running;
    std::thread m_worker;

  public:
    ProgressBar() : m_total{0}, m_current{0}, m_width{0}, m_running(true) {
        this->run();
    }

    ProgressBar(uint64_t total, uint16_t width)
        : m_total{total}, m_current{0}, m_width{width}, m_running(true) {
        this->run();
    }

    ~ProgressBar() { this->stop(); }

    void add(uint64_t value) { m_current += value; }

    void stop() {
        if (this->m_worker.joinable()) {
            this->m_running = false;
            m_worker.join();
        }
        std::cout << "\n";
    }

  private:
    void run() {
        const std::chrono::milliseconds interval =
            std::chrono::milliseconds(500);
        const std::chrono::steady_clock::time_point start =
            std::chrono::steady_clock::now();

        m_worker = std::thread([=]() {
            while (m_running == true) {
                auto progress = (float)m_current / m_total;
                auto pos = (uint)(m_width * progress);

                auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count();

                std::cout << "[";

                for (uint i = 0; i < m_width; ++i) {
                    if (i < pos)
                        std::cout << "-"; // complete char
                    else if (i == pos)
                        std::cout << ">"; // current char
                    else
                        std::cout << "_"; // incomplete char
                }
                std::cout << "] " << int(progress * 100.0) << "% "
                          << (float(m_current) / 1000) / float(elapsed)
                          << " MB p/s\r";
                std::cout.flush();

                std::this_thread::sleep_for(interval);
            }
        });
    }
};
} // namespace xrdndnconsumer

#endif // XRDNDN_CONSUMER_PROGRESS_BAR_HH
