#pragma once

#include "MoCapLib.h"

namespace TD_MoCap
{
	struct TriangulateParameters : Utils::Parameters {
		Utils::PathParameter leftCameraCalibration {
			"Left camera calibration"
			, Utils::PathParameter::Type::File
		};

		Utils::PathParameter rightCameraCalibration{
			"Right camera calibration"
			, Utils::PathParameter::Type::File
		};

		Utils::PathParameter stereoCameraCalibration{
			"Stereo camera calibration"
			, Utils::PathParameter::Type::File
		};

		Utils::ValueParameter<bool> includeTestData{
			"Include test data"
			, false, false
		};

		Utils::NumberParameter<float> angleThreshold{
			"Angle threshold", "degrees"
			, 2, 2
			, 0.001, 180
			, 0.001, 180
		};

		Utils::NumberParameter<float> epipolarLineThreshold{
			"Epipolar line distance", "px"
			, 3, 3
			, 0, 1024
			, 0.001, 16
		};

		Utils::NumberParameter<float> massRatioThreshold{
			"Mass ratio threshold", "x"
			, 5, 5
			, 1, 1024
			, 1, 16
		};

		Utils::NumberParameter<float> intersectDistanceThreshold{
			"Intersect distance threshold", "m"
			, 0.05, 0.05
			, 0.001, 1
			, 0, 1024
		};

		Utils::NumberParameter<float> zMinimum{
			"Z minimum", "m"
			, 0.1, 0.1
			, -100, 100
			, 0.0, 10.0
		};

		Utils::NumberParameter<float> zMaximum{
			"Z maximum", "m"
			, 2.0, 2.0
			, -100, 100
			, 0.0, 10.0
		};

		Utils::ParameterList list{
			&leftCameraCalibration
			, &rightCameraCalibration
			, &stereoCameraCalibration
			, &includeTestData
			, &angleThreshold
			, &epipolarLineThreshold
			, &massRatioThreshold
			, &intersectDistanceThreshold
			, &zMinimum
			, &zMaximum
		};

		void update() override;

		Math::Camera cameraLeft, cameraRight;
		cv::Mat fundamental_matrix;
		cv::Point2f cameraLeftInCameraRight;
	};

	class Process_Triangulate
		: public IProcessor<Frames::CentroidsFrame, Frames::TriangulateFrame, TriangulateParameters>
	{
	public:
		void process(std::shared_ptr<Frames::CentroidsFrame>, std::shared_ptr<Frames::TriangulateFrame>, TriangulateParameters&) override;
	};
}