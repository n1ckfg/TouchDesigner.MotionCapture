#pragma once
#include <map>
#include "MoCapLib.h"

#include "Frames/CameraFrame.h"

namespace TD_MoCap {
	/// <summary>
	/// Synchronise incoming frames. Note this class does not use locks, we only use perform/Blocking
	/// </summary>
	class Synchroniser
	{
	public:
		struct SyncMember
		{
			Links::Input input;
			std::map<uint64_t, std::shared_ptr<Frames::CameraFrame>> indexedFrames;

			uint64_t frameNumberStart = 0;
			std::chrono::microseconds timestampStart;
		};
		Synchroniser();
		~Synchroniser();

		void checkConnections(const std::vector<Links::Output::ID>&);
		void requestResync();
		void sendSynchronisedFrames();

		Utils::WorkerThread& getThread();
		Utils::ParameterList& getParameters();

		Links::Output output;

		// send a message on this channel (to clear errors in main interface)
		Utils::ThreadChannel<bool> syncSuccess;
	protected:
		void threadedUpdate();

		void receiveAllFrames();
		void resync();

		bool needsResync = true;

		Utils::WorkerThread workerThread;

		std::map<Links::Output::ID, std::unique_ptr<SyncMember>> syncMembers;
		Links::Output::ID leaderID;

		const bool checkCameraTriggers = false;

		struct
		{
			Utils::NumberParameter<int> maxHistory
			{
				"History size", "frames"
				, 10, 10
				, 1, 10000
				, 1, 20
			};

			Utils::SelectorParameter strategy
			{
				"Strategy"
				, {"Frameindex", "Timestamp"}
				, "Frameindex"
			};

			Utils::NumberParameter<float> timestampWindow
			{
				"Timestamp window", "ms"
				, 1000.0f / 120.0f, 1000.0f / 120.0f
				, 0, 1000.0f
			};

			Utils::ParameterList list{ &maxHistory, &strategy, &timestampWindow};
		} parameters;
	};
}