#include "pch_MoCapLib.h"
#include "Camera.h"
#include "Exception.h"

namespace TD_MoCap {
	namespace Math {
		//----------
		Camera::Camera()
		{
			this->translation = cv::Mat::zeros(3, 1, CV_32F);
			this->rotationMatrix = cv::Mat::eye(3, 3, CV_32F);
			this->updateViewMatrix();
		}

		//----------
		void
			Camera::load(const std::filesystem::path& path)
		{
			if (!path.empty()) {
				cv::FileStorage fs(path.string(), cv::FileStorage::READ);
				cv::Mat cameraMatrix, distortionCoefficients;
				fs["camera_matrix"] >> cameraMatrix;
				fs["distortion_coefficients"] >> distortionCoefficients;

				cv::Size size;
				{
					fs["image_width"] >> size.width;
					fs["image_height"] >> size.height;
				}

				this->setIntrinsics(cameraMatrix, distortionCoefficients, size);
			}
		}

		//----------
		void
			Camera::setIntrinsics(const cv::Mat& cameraMatrix, const cv::Mat& distortionCoefficients, const cv::Size& size)
		{
			this->cameraMatrix = cameraMatrix;
			this->distortionCoefficients = distortionCoefficients;
			this->size = size;

			this->cameraMatrix.convertTo(this->cameraMatrix, CV_32F);
			this->distortionCoefficients.convertTo(this->distortionCoefficients, CV_32F);

			this->updateProjectionMatrix();
		}

		//----------
		void
			Camera::setExtrinsics(const cv::Mat& rotationMatrix, const cv::Mat& translation)
		{
			if (rotationMatrix.empty() || translation.empty()) {
				throw(Exception("Cannot set extrinsics with empty data"));
			}
			this->rotationMatrix = rotationMatrix;
			this->translation = translation;

			this->rotationMatrix.convertTo(this->rotationMatrix, CV_32F);
			this->translation.convertTo(this->translation, CV_32F);

			this->updateViewMatrix();
		}

		//----------
		const cv::Mat &
			Camera::getCameraMatrix() const
		{
			return this->cameraMatrix;
		}

		//----------
		const cv::Mat&
			Camera::getDistortionCoefficients() const
		{
			return this->distortionCoefficients;
		}

		//----------
		bool
			Camera::empty() const
		{
			return this->cameraMatrix.empty() || this->distortionCoefficients.empty();
		}

		//----------
		glm::vec3
			Camera::getPosition() const
		{
			return this->position;
		}

		//----------
		glm::mat4
			Camera::getProjectionMatrix() const
		{
			return this->projectionMatrix;
		}

		//----------
		glm::mat4
			Camera::getClippedProjectionMatrix() const
		{
			return this->clippedProjectionMatrix;
		}

		//----------
		glm::mat4
			Camera::getViewMatrix() const
		{
			return this->viewMatrix;
		}

		//----------
		glm::mat4
			Camera::getInverseViewProjection() const
		{
			return this->inverseViewProjection;
		}

		//----------
		std::vector<cv::Point2f>
			Camera::undistortImagePoints(const std::vector<cv::Point2f>& imagePoints, size_t roiY) const
		{
			// adjust for roi
			auto imagePointsActual = imagePoints;
			for (auto& imagePointActual : imagePointsActual) {
				imagePointActual.y += roiY;
			}

			// undistort
			std::vector<cv::Point2f> imagePointsIdeal;
			cv::undistortPoints(imagePointsActual
				, imagePointsIdeal
				, this->cameraMatrix
				, this->distortionCoefficients);

			// remap to pixel coordinates
			std::vector<cv::Point2f> undistortedImagePoints;
			{
				undistortedImagePoints.reserve(imagePoints.size());

				const auto& f_x = this->cameraMatrix.at<float>(0, 0);
				const auto& f_y = this->cameraMatrix.at<float>(1, 1);
				const auto& c_x = this->cameraMatrix.at<float>(0, 2);
				const auto& c_y = this->cameraMatrix.at<float>(1, 2);

				for (const auto& imagePointIdeal : imagePointsIdeal) {
					undistortedImagePoints.emplace_back(
						imagePointIdeal.x * f_x + c_x
						, imagePointIdeal.y * f_y + c_y
					);
				}
			}

			return undistortedImagePoints;
		}

		//----------
		std::vector<Ray>
			Camera::unprojectUndistortedImagePoints(const std::vector<cv::Point2f>& undistortedImagePoints) const
		{
			auto coordinates = this->undistortedImagePointsToCoordinates(undistortedImagePoints);
			std::vector<Ray> rays;
			rays.reserve(coordinates.size());
			for (const auto& coordinate : coordinates) {
				rays.push_back(this->unprojectCoordinate(coordinate));
			}
			return rays;
		}

		//----------
		Ray
			Camera::unprojectCoordinate(const cv::Point2f& imageCoordinate) const
		{
			auto inverseViewProjection = this->getInverseViewProjection();
			auto posW = inverseViewProjection * glm::vec4(imageCoordinate.x, imageCoordinate.y, +1.0f, 1.0f);
			posW /= posW.w;

			auto s = this->getPosition();
			auto t = glm::normalize((glm::vec3)posW - s);

			return Ray(s, t);
		}

		//----------
		std::vector<cv::Point2f>
			Camera::undistortedImagePointsToCoordinates(const std::vector<cv::Point2f>& undistortedImagePoints) const
		{
			// remap to coordinates (note these 2 stages can be merged later)
			std::vector<cv::Point2f> coordinates;
			{
				auto width = (float)this->size.width;
				auto height = (float)this->size.height;

				coordinates.reserve(undistortedImagePoints.size());
				for (const auto& undistortedImagePoint : undistortedImagePoints) {
					coordinates.emplace_back(
						(undistortedImagePoint.x + 0.5f) / width * 2.0f - 1.0f
						, 1.0f - (undistortedImagePoint.y + 0.5f) / height* 2.0f
					);
				}
			}

			return coordinates;
		}

		//----------
		void
			Camera::updateProjectionMatrix()
		{
			// Projection Matrix
			{
				float f_x = this->cameraMatrix.at<float>(0, 0);
				float f_y = this->cameraMatrix.at<float>(1, 1);
				float c_x = this->cameraMatrix.at<float>(0, 2);
				float c_y = this->cameraMatrix.at<float>(1, 2);

				auto width = (float)this->size.width;
				auto height = (float)this->size.height;


				this->projectionMatrix = glm::identity<glm::mat4>();
				this->projectionMatrix[0][0] = 2.0f * f_x / (float) width;
				this->projectionMatrix[1][1] = -2.0f * f_y / (float) height;
				this->projectionMatrix[2][3] = 1.0f;
				this->projectionMatrix[3][3] = 0.0f;

				const glm::vec3 lensOffset(
					2.0f * c_x / width - 1.0f
					, 1.0f - 2.0f * c_y / height
					, 0.0f);

				this->projectionMatrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(lensOffset.x, lensOffset.y, 0)) * this->projectionMatrix;
			}

			// Clipped projection matrix
			{
				this->clippedProjectionMatrix = this->projectionMatrix;
				
				const auto n = 0.05f; // near clip
				const auto f= 1.0f; // far clip

				this->clippedProjectionMatrix[2][2] *= (f + n) / (f - n);
				this->clippedProjectionMatrix[3][2] = -(f * n / (f - n));
			}

			// Inverse view projection
			{
				this->inverseViewProjection = glm::inverse(this->getClippedProjectionMatrix() * this->getViewMatrix());
			}
		}

		//----------
		void
			Camera::updateViewMatrix()
		{
			// Update view matrix
			{
				float* rm = this->rotationMatrix.ptr<float>(0);
				float* tm = this->translation.ptr<float>(0);
				this->viewMatrix = glm::mat4(rm[0], rm[3], rm[6], 0.0f,
					rm[1], rm[4], rm[7], 0.0f,
					rm[2], rm[5], rm[8], 0.0f,
					tm[0], tm[1], tm[2], 1.0f);
			}

			// Position
			{
				auto posW4 = glm::inverse(this->viewMatrix) * glm::vec4(0, 0, 0, 1);
				this->position = (glm::vec3) posW4 / posW4.w;
			}

			// Inverse view projection
			{
				this->inverseViewProjection = glm::inverse(this->getClippedProjectionMatrix() * this->getViewMatrix());
			}
		}
	}
}