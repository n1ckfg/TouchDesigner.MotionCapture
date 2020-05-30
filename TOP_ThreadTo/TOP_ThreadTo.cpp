#include "pch_TOP_ThreadTo.h"
#include "TOP_ThreadTo.h"

namespace TD_MoCap {
	//----------
	TOP_ThreadTo::TOP_ThreadTo(const OP_NodeInfo* info)
	{
	}

	//----------
	TOP_ThreadTo::~TOP_ThreadTo()
	{
	}

	//----------
	void
		TOP_ThreadTo::getGeneralInfo(TOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void*)
	{
		ginfo->cookEveryFrame = true;
		ginfo->memFirstPixel = TOP_FirstPixel::TopLeft;
		ginfo->memPixelType = OP_CPUMemPixelType::R8Fixed;
	}

	//----------
	bool
		TOP_ThreadTo::getOutputFormat(TOP_OutputFormat* format, const OP_Inputs* inputs, void* reserved1)
	{
		auto frame = this->input.receiveLatestFrame(false);
		if (frame) {
			// new frame received
			this->previewDirty = true;
		}
		else {
			// no new frame
			frame = this->input.getLastFrame();
		}

		if (frame) {
			cv::Mat image;
			if (frame->getPreviewImage(image)) {
				setTOP_OutputFormat(format, image);

				// make a copy for comparison later
				this->allocatedOutputFormat = *format;
				return true;
			}
		}

		// in all other cases
		return false;
	}

	//----------
	void
		TOP_ThreadTo::execute(TOP_OutputFormatSpecs* output,
			const OP_Inputs* inputs,
			TOP_Context* context,
			void* reserved1)
	{
		// update input stats
		this->input.update(inputs->getParDAT("Source"));

		if (this->previewDirty) {
			// We do not receive any new frames here, as we need to ensure that the frame is the same as the one seen at teh time of getOutputFormat
			auto frame = this->input.getLastFrame();

			cv::Mat image;
			if (!frame->getPreviewImage(image)) {
				// this frame doesn't support image output
				return;
			}

			if (image.empty()) {
				return;
			}

			TOP_OutputFormat requiredOutputFormat = this->allocatedOutputFormat; // initialise all elements the same that we don't care about
			setTOP_OutputFormat(&requiredOutputFormat, image);
			if (memcmp(&requiredOutputFormat, &this->allocatedOutputFormat, sizeof(requiredOutputFormat))) {
				// wait until next frame
				return;
			}

			// copy it into the gpu memory
			output->newCPUPixelDataLocation = 0;
			memcpy(output->cpuPixelData[0]
				, image.data
				, image.total() * image.elemSize());

			this->previewDirty = false;
		}
	}

	//----------
	void
		TOP_ThreadTo::setupParameters(OP_ParameterManager* manager, void* reserved1)
	{
		{
			OP_StringParameter param;

			param.name = "Source";
			param.label = param.name;

			auto res = manager->appendDAT(param);
			assert(res == OP_ParAppendResult::Success);
		}
	}

	//----------
	void
		TOP_ThreadTo::pulsePressed(const char* name, void* reserved1)
	{
	}
}