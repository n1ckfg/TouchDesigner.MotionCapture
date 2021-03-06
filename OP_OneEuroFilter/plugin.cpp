#include "pch_OP_OneEuroFilter.h"
#include "Process_OneEuroFilter.h"

extern "C"
{

	DLLEXPORT
		void
		FillDATPluginInfo(DAT_PluginInfo* info)
	{
		// Always return DAT_CPLUSPLUS_API_VERSION in this function.
		info->apiVersion = DATCPlusPlusAPIVersion;

		// The opType is the unique name for this TOP. It must start with a
		// capital A-Z character, and all the following characters must lower case
		// or numbers (a-z, 0-9)
		info->customOPInfo.opType->setString("Oneeurofilter");

		// The opLabel is the text that will show up in the OP Create Dialog
		info->customOPInfo.opLabel->setString("MoCap One Euro Filter");

		// Will be turned into a 3 letter icon on the nodes
		info->customOPInfo.opIcon->setString("OEF");

		// Information about the author of this OP
		info->customOPInfo.authorName->setString("Elliot Woods");
		info->customOPInfo.authorEmail->setString("elliot@kimchiandchips.com");

		info->customOPInfo.minInputs = 1;
		info->customOPInfo.maxInputs = 1;
	}

	DLLEXPORT
		DAT_CPlusPlusBase*
		CreateDATInstance(const OP_NodeInfo* info)
	{
		// Return a new instance of your class every time this is called.
		// It will be called once per DAT that is using the .dll
		return new TD_MoCap::ThreadedOP<TD_MoCap::Process_OneEuroFilter>(info);
	}

	DLLEXPORT
		void
		DestroyDATInstance(DAT_CPlusPlusBase* instance)
	{
		// Delete the instance here, this will be called when
		// Touch is shutting down, when the DAT using that instance is deleted, or
		// if the DAT loads a different DLL
		delete (TD_MoCap::ThreadedOP<TD_MoCap::Process_OneEuroFilter>*)instance;
	}

};