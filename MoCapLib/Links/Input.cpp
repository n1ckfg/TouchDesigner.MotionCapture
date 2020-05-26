#include "pch_MoCapLib.h"
#include "Input.h"
#include "OutputsRegister.h"


namespace TD_MoCap {
	namespace Links {
		//----------
		Input::~Input()
		{
			this->unsubscribe();
		}

		//----------
		void
			Input::update(const OP_DATInput* dat)
		{
			Output::ID newID;

			if (!Output::getIDFromDAT(dat, newID)) {
				this->unsubscribe();
			}

			// if we stayed the same, ignore it
			if (this->connectedOutput) {
				if (connectedOutput->getID() == newID) {
					return;
				}
			}

			this->subscribe(newID);
		}

		//----------
		void
			Input::subscribe(Output::ID id)
		{
			this->unsubscribe();

			auto output = OutputsRegister::X().findByID(id);
			if (output) {
				this->connectedOutput = output;
				output->addSubscriber(this);
			}
		}

		//----------
		void
			Input::unsubscribe()
		{
			if (!this->connectedOutput) {
				return;
			}
			else {
				this->connectedOutput->removeSubscriber(this);
				this->connectedOutput = nullptr;
			}
		}

		//----------
		void
			Input::send(std::shared_ptr<BaseFrame> frame)
		{
			this->channel.send(frame);
		}

		//----------
		std::shared_ptr<BaseFrame> 
			Input::receiveNextFrame(bool waitForFrame)
		{
			std::shared_ptr<BaseFrame> frame;

			if (waitForFrame) {
				this->channel.receive(frame);
			}
			else {
				this->channel.tryReceive(frame);
			}

			if (frame) {
				this->lastFrame = frame;
			}

			return frame;
		}

		//----------
		std::shared_ptr<BaseFrame>
			Input::receiveLatestFrame(bool useCached)
		{
			std::shared_ptr<BaseFrame> frame;
			while (this->channel.tryReceive(frame)) {
				// flush until latest	
			}

			if (frame) {
				this->lastFrame = frame;
				return frame;
			}
			else if (useCached) {
				return lastFrame;
			}
		}
	}
}