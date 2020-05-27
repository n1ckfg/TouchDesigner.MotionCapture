#include "pch_DAT_XimeaCameras.h"
#include "DAT_XimeaCameras.h"
#include <xiAPIplus/xiapiplus.h>

namespace TD_MoCap {
	//----------
	DAT_XimeaCameras::DeviceInfo::DeviceInfo()
	{
		// initialise to empty strings

		this->serialNumber[0] = '\0';
		this->name[0] = '\0';
		this->instancePath[0] = '\0';
		this->locationPath[0] = '\0';
		this->type[0] = '\0';
	}

	//----------
	DAT_XimeaCameras::DAT_XimeaCameras(const OP_NodeInfo* info)
	{
	}

	//----------
	DAT_XimeaCameras::~DAT_XimeaCameras()
	{

	}

	//----------
	void
		DAT_XimeaCameras::getGeneralInfo(DAT_GeneralInfo* info, const OP_Inputs*, void* reserved1)
	{
		info->cookEveryFrame = false;
	}

	//----------
	void
		DAT_XimeaCameras::execute(DAT_Output* output, const OP_Inputs* inputs, void* reserved)
	{
		try {

			if (this->needsRefresh) {
				DWORD deviceCount;
				auto result = xiGetNumberDevices(&deviceCount);
				if (result != XI_OK) {
					throw(Exception("Failed to enumerate devices"));
				}

				Utils::Table table;
				table.newRow() << "id" << "serial" << "name" << "instancePath" << "locationPath" << "type";

				auto checkResult = [this](XI_RETURN result) {
					if (result != XI_OK) {
						try {
							ReportException(result, "Get device info");
						}
						catch (xiAPIplus_Exception e) {
							char message[100];
							e.GetDescription(message, sizeof(message));
							throw(Exception(message));
						}
					}
				};

				this->deviceInfos.clear();
				this->deviceInfos.reserve(deviceCount);
				for (DWORD i = 0; i < deviceCount; i++) {
					DeviceInfo deviceInfo;

					// ignore errors here (e.g. invalid results)
					checkResult(xiGetDeviceInfoString(i
						, XI_PRM_DEVICE_SN
						, deviceInfo.serialNumber
						, sizeof(deviceInfo.serialNumber)));
					checkResult(xiGetDeviceInfoString(i
						, XI_PRM_DEVICE_NAME
						, deviceInfo.name
						, sizeof(deviceInfo.name)));
					checkResult(xiGetDeviceInfoString(i
						, XI_PRM_DEVICE_INSTANCE_PATH
						, deviceInfo.instancePath
						, sizeof(deviceInfo.instancePath)));
					checkResult(xiGetDeviceInfoString(i
						, XI_PRM_DEVICE_LOCATION_PATH
						, deviceInfo.locationPath
						, sizeof(deviceInfo.locationPath)));
					checkResult(xiGetDeviceInfoString(i
						, XI_PRM_DEVICE_TYPE
						, deviceInfo.type
						, sizeof(deviceInfo.type)));

					table.newRow()
						<< i
						<< deviceInfo.serialNumber
						<< deviceInfo.name
						<< deviceInfo.instancePath
						<< deviceInfo.locationPath
						<< deviceInfo.type;
				}
				table.populateOutput(output);
			}
		}
		catch (const Exception& e) {
			this->errors.push_back(e);
		}
	}

	//----------
	int32_t
		DAT_XimeaCameras::getNumInfoCHOPChans(void* reserved1)
	{
		return 0;
	}

	//----------
	void
		DAT_XimeaCameras::getInfoCHOPChan(int index, OP_InfoCHOPChan* chan, void* reserved1)
	{
	}

	//----------
	bool
		DAT_XimeaCameras::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
	{
		return false;
	}

	//----------
	void
		DAT_XimeaCameras::getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1)
	{
	}

	//----------
	void
		DAT_XimeaCameras::setupParameters(OP_ParameterManager* manager, void* reserved1)
	{

	}

	//----------
	void
		DAT_XimeaCameras::pulsePressed(const char* name, void* reserved1)
	{
		if (strcmp(name, "Refresh") == 0) {
			this->needsRefresh = true;
		}
	}

	//----------
	void
		DAT_XimeaCameras::getErrorString(OP_String* error, void* reserved1)
	{
		if (!this->errors.empty()) {
			std::string errorString;
			for (const auto& error : this->errors) {
				errorString += error.what() +"\n";
			}
			error->setString(errorString.c_str());
		}
	}
}
