#pragma once

#include <stdint.h>
#include <chrono>

namespace TD_MoCap {
	namespace Utils {
		class TDMOCAP_API FrameRateCounter
		{
		public:
			FrameRateCounter(uint16_t windowSize = 100);
			~FrameRateCounter();

			void clear();
			void tick();
			float getFPS() const;
			float getFPSUnfiltered() const;
		protected:
			uint16_t position = 0;
			uint16_t firstRun = 0;
			uint16_t windowSize;
			std::chrono::high_resolution_clock::time_point* data;
		};
	}
}