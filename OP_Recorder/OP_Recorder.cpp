#include "pch_OP_Recorder.h"
#include "OP_Recorder.h"

#include <time.h>
#include <codecvt>

namespace TD_MoCap {
	//----------
	OP_Recorder::OP_Recorder(const OP_NodeInfo* info)
	{

	}

	//----------
	OP_Recorder::~OP_Recorder()
	{
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
		auto recordFolder = std::string(inputs->getParFilePath("Folder"));

		if (inputs->getNumInputs() < 1) {
			return;
		}

		this->input.update(inputs->getInputDAT(0));
		this->parameters.list.updateFromInterface(inputs);

		// start / stop recording
		if (this->isRecording != this->parameters.record.getValue()) {
			if (this->parameters.record.getValue()) {
				this->outputPath.assign(recordFolder);
				this->startRecording();
			}
			else {
				this->stopRecording();
			}
		}

		// update recording
		if (this->isRecording) {
			this->updateRecording();
		}
		else {
			// always clear out input if we're not processing - else memory use will explode
			this->input.getChannel().clear();
		}

		// update enabled parameters
		{
			inputs->enablePar(this->parameters.record.getTDShortName().c_str(), !recordFolder.empty() || this->isRecording);
			inputs->enablePar(this->parameters.play.getTDShortName().c_str(), !this->parameters.record.getValue());
		}
	}

	//----------
	int32_t
		OP_Recorder::getNumInfoCHOPChans(void* reserved1)
	{
		return 2;
	}

	//----------
	void
		OP_Recorder::getInfoCHOPChan(int index, OP_InfoCHOPChan* chan, void* reserved1)
	{
		switch (index) {
		case 0:
		{
			chan->name->setString("Workqueuelength");
			chan->value = (float)Utils::WorkerGroup::X().sizeWorkItems();
		}
		break;
		case 1:
		{
			chan->name->setString("Recordframerate");
			chan->value = this->recordFrameRateCounter.getFPS();
		}
		break;
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

		this->parameters.list.populateInterface(manager);
	}

	//----------
	void
		OP_Recorder::pulsePressed(const char* name, void* reserved1)
	{

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

	//----------
	void
		OP_Recorder::startRecording()
	{
		this->stopRecording();

		this->recordingJson.clear();
		this->isRecording = true;
		this->recordFrameRateCounter.clear();

		this->saveArgs.frameIndex = 0;
		this->saveArgs.folderOut = this->outputPath;
		this->saveArgs.imageFormat = this->parameters.format.getValue();
		this->saveArgs.onComplete = [this] {
			this->recordFrameRateCounter.tick();
		};

		this->recordingJson["frames"] = nlohmann::json::array();
		this->recordingJson["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

		{
			TCHAR buffer[256];
			DWORD dwSize = sizeof(buffer);
			if (GetComputerNameEx(COMPUTER_NAME_FORMAT::ComputerNameNetBIOS, buffer, &dwSize)) {
				std::wstring wideString(buffer);
				this->recordingJson["computer"] = std::string(wideString.begin(), wideString.end());
			}
		}
		{
			time_t rawTime;
			char buffer[80];
			time(&rawTime);
			struct tm timeInfo;
			localtime_s(&timeInfo, &rawTime);

			strftime(buffer, sizeof(buffer), "%F %T (%Z)", &timeInfo);
			this->recordingJson["dateString"] = std::string(buffer);
		}
	}

	//----------
	void
		OP_Recorder::stopRecording()
	{
		if (!this->isRecording) {
			return;
		}

		auto jsonFilePath = outputPath / "recording.json";
		std::ofstream file(jsonFilePath.string());
		file << this->recordingJson;
		file.close();
		this->isRecording = false;

		this->recordingJson.clear();
	}

	//----------
	void
		OP_Recorder::updateRecording()
	{
		auto frame = this->input.receiveNextFrameDontWait();
		while (frame) {
			// check if we're out of space
			if (Utils::WorkerGroup::X().sizeWorkItems() > this->parameters.maxQueueLength.getValue()) {
				this->errors.push_back(Exception("Work queue length exceeded"));
				this->input.getChannel().clear();
				return;
			}

			// perform the save
			{
				nlohmann::json jsonFrame;
				frame->serialise(jsonFrame, this->saveArgs);
				frame = this->input.receiveNextFrameDontWait();
				this->recordingJson["frames"].push_back(jsonFrame);

				this->saveArgs.frameIndex++;
			}
		}
	}
}
