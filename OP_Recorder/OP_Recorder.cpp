#include "pch_OP_Recorder.h"
#include "OP_Recorder.h"
#include "SynchronisedFrame.h"

namespace TD_MoCap {
	//----------
	OP_Recorder::OP_Recorder(const OP_NodeInfo* info)
	{
	}

	//----------
	OP_Recorder::~OP_Recorder()
	{
		this->workGroup.close();
	}

	//----------
	void
		OP_Recorder::getGeneralInfo(DAT_GeneralInfo* info, const OP_Inputs*, void* reserved1)
	{
		info->cookEveryFrame = true;
	}

	//----------
	void
		OP_Recorder::execute(DAT_Output* output, const OP_Inputs* inputs, void* reserved)
	{
		if (inputs->getNumInputs() < 1) {
			return;
		}

		this->input.update(inputs->getInputDAT(0));
		this->parameters.list.updateFromInterface(inputs);
		
		// iterate through all incoming frames in the main loop
		auto frame = this->input.receiveNextFrame(false);
		auto recordFolder = std::string(inputs->getParFilePath("Folder"));
		while (frame) {
			auto typedFrame = std::dynamic_pointer_cast<SynchronisedFrame>(frame);

			// if it's a valid frame and we are set to record
			if (typedFrame && this->parameters.record.getValue()) {
				// if we we have a frame, dispatch recording it to worker thread group
				std::string filePath(inputs->getParFilePath("Folder"));
				if (!filePath.empty()) {
					this->workGroup.perform([typedFrame, filePath] {
						typedFrame->save(std::filesystem::path(filePath));
					});
				}
			}
			frame = this->input.receiveNextFrame(false);
		}

		// get errors from threads
		{
			Exception e;
			while (this->workGroup.errorsInThreads.tryReceive(e)) {
				this->errors.push_back(e);
			}
		}

		// upadte parameters
		{
			inputs->enablePar(this->parameters.record.getTDShortName().c_str(), !recordFolder.empty());
			inputs->enablePar(this->parameters.play.getTDShortName().c_str(), !this->parameters.record.getValue());
		}
	}

	//----------
	int32_t
		OP_Recorder::getNumInfoCHOPChans(void* reserved1)
	{
		return 1;
	}

	//----------
	void
		OP_Recorder::getInfoCHOPChan(int index, OP_InfoCHOPChan* chan, void* reserved1)
	{
		switch (index) {
		case 0:
		{
			chan->name->setString("Workqueuelength");
			chan->value = (float) this->workGroup.sizeWorkItems();
		}
		default:
			break;
		}
	}

	//----------
	bool
		OP_Recorder::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
	{
		return 1;
	}

	//----------
	void
		OP_Recorder::getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1)
	{

	}

	//----------
	void
		OP_Recorder::setupParameters(OP_ParameterManager* manager, void* reserved1)
	{
		// folder
		{
			OP_StringParameter param;

			param.name = "Folder";
			param.label = "Recording folder";
			
			auto res = manager->appendFolder(param);
			assert(res == OP_ParAppendResult::Success);
		}

		// clear
		{
			OP_NumericParameter param;

			param.name = "Clearqueue";
			param.label = "Clear queue";

			auto res = manager->appendPulse(param);
			assert(res == OP_ParAppendResult::Success);
		}

		this->parameters.list.populateInterface(manager);
	}

	//----------
	void
		OP_Recorder::pulsePressed(const char* name, void* reserved1)
	{
		if (strcmp(name, "Clearqueue") == 0) {
			this->workGroup.clearWorkItems();
		}
	}

	//----------
	void
		OP_Recorder::getErrorString(OP_String* error, void* reserved1)
	{
		if (!this->errors.empty()) {
			std::string errorString;
			for (const auto& error : this->errors) {
				errorString += error.what() + "\n";
			}
			error->setString(errorString.c_str());
		}

		this->errors.clear();
	}
}