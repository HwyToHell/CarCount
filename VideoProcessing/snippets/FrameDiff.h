#include "stdafx.h"
#include "../include/Tracking.h"

class FrameDiff
{
public:
	FrameDiff();
	void apply(const cv::Mat& frame, cv::Mat& fgMask);
	void updateBg(Scene& scene, std::vector<std::vector<cv::Point>>& contours, cv::Mat& bgModel);
private:
	cv::Mat m_bgModel_act;
	cv::Mat m_bgModel_prev;
	cv::Mat m_frame_act;
	cv::Mat m_frame_prev;
	cv::Mat m_fgMask;
	double m_fadingFactor;
	bool m_enableThreshold;
	int m_threshold;
};