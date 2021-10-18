/*
* N-DISE: NDN for Data Intensive Science Experiments
* Author: Sichen Song <songsichen@cs.ucla.edu>
*
* MIT License
*
* Copyright (c) 2021 California Institute of Technology
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef NDNC_PIPELINE_INTERESTS_AIMD_HPP
#define NDNC_PIPELINE_INTERESTS_AIMD_HPP

#include "pipeline-interests.hpp"

namespace ndnc {
class PipelineInterestsAimd : public PipelineInterests {
 public:
   PipelineInterestsAimd(Face &face, size_t size);
   ~PipelineInterestsAimd();

 private:
   void process() final;

   void onData(std::shared_ptr<ndn::Data> &&data,
               ndn::lp::PitToken &&pitToken) final;

   void onNack(std::shared_ptr<ndn::lp::Nack> &&nack,
               ndn::lp::PitToken &&pitToken) final;

   void onTimeout() final;

   void decreaseWindow();

   void increaseWindow();

 private:
   size_t m_windowSize;
   ndn::time::steady_clock::time_point m_lastDecrease;
   static const unsigned MAX_WINDOW = NDNC_MAX_MEMIF_BUFS;
   static const unsigned MIN_WINDOW = 10;
   constexpr static const ndn::time::milliseconds MAX_RTT = ndn::time::milliseconds{3};
};
}; // namespace ndnc

#endif // NDNC_PIPELINE_INTERESTS_AIMD_HPP
