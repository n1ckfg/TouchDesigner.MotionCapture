#pragma once

#include <string>
#include "Utils/Table.h"
#include "Utils/Serialisable.h"

namespace TD_MoCap {
	namespace Frames {

		class TDMOCAP_API BaseFrame : public Utils::Serialisable
		{
		public:
			virtual ~BaseFrame() {

			}

			virtual std::string getTypeName() const = 0;

			virtual bool getPreviewImage(cv::Mat&) const {
				return false;
			}

			virtual bool getPreviewDAT(Utils::Table&) const {
				return false;
			}

			void startComputeTimer();
			void endComputeTimer();
			std::chrono::high_resolution_clock::duration getComputeTime() const;
			void setComputeTime(const std::chrono::high_resolution_clock::duration&);

			virtual uint64_t getFrameIndex() const = 0;
		protected:
			std::chrono::high_resolution_clock::time_point startCompute;
			std::chrono::high_resolution_clock::duration computeTime;
		};
	}
}