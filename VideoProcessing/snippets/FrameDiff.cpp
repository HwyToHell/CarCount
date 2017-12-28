#include "stdafx.h"
#include "FrameDiff.h"


FrameDiff::FrameDiff() : m_fadingFactor(10), m_threshold(15) 
{}

void FrameDiff::apply(const cv::Mat& frame, cv::Mat& fgMask)
{
	const int white_8UC1 = 255;
	// convert input frame to class internal grey representation m_frame_act
	cv::cvtColor(frame, m_frame_act, CV_BGR2GRAY);

	if(m_bgModel_act.empty())
		m_frame_act.copyTo(m_bgModel_act);
	if(m_bgModel_prev.empty())
		m_frame_act.copyTo(m_bgModel_prev);

	cv::absdiff(m_frame_act, m_bgModel_act, m_fgMask);
	cv::threshold(m_fgMask, m_fgMask, m_threshold, white_8UC1, cv::THRESH_BINARY);
	fgMask = m_fgMask.clone(); // remove after debug, not needed outside member fcn	
}


/* updateBackground to be called after FrameDiff::apply and Scene::CombineTracks
 * generate background frame from previous foreground with first order low pass filter
 * moving objects are masked using the two following methods:
 * 1. scene contains moving objects with high confidence: 
 *	  - use moving contours to build fgMask 
 *    - prevents everlasting ghosts due to light change)
 * 2. no moving objects in scene: 
 *    - use last fgMask
 *    - prevents ghosts behind moving objects
 */
void FrameDiff::updateBg(Scene& scene, std::vector<std::vector<cv::Point>>& contours, cv::Mat& bgModel)
{
	const int white_8UC1 = 255;
	cv::Mat bgMask = white_8UC1 - m_fgMask;
	cv::Mat fgMask = m_fgMask;


	/*cv::Mat medBlur;
	cv::medianBlur(m_fgMask, medBlur, 5);
	cv::imshow("fg mask blur", medBlur);
	*/

	// relative frame diff
	/*if(m_frame_prev.empty())
		m_frame_act.copyTo(m_frame_prev);
	cv::Mat diffFgMask;
	cv::absdiff(m_frame_act, m_frame_prev, diffFgMask);
	cv::threshold(diffFgMask, diffFgMask, m_threshold, white_8UC1, cv::THRESH_BINARY);
	cv::imshow("rel diff bg", diffFgMask);
	m_frame_prev = m_frame_act;
	*/

	// debug_begin
	// debug_end
	cv::Mat fg_act_masked, bg_act_masked, bg_model_act;

	// ToDo: 
	// IF MOTION: use contour indices to create bgMask
	// NO MOTION: use bgMask
	std::vector<int> contourIdx = scene.getAllContourIndices();
	unsigned int nContours = contourIdx.size();
	if (nContours > 0)
	{
		// build mask from moving contours
		cv::Mat fgMovingMask(bgMask.size(), CV_8UC1, cv::Scalar(0));
		for (unsigned int i = 0; i < nContours; ++i)
		{
			int idx = contourIdx[i];
			cv::drawContours(fgMovingMask, contours, idx, white_8UC1, CV_FILLED, 8);
		}
		// debug_begin

		fgMask = fgMask & fgMovingMask;
		cv::imshow("_debug_fgMask", fgMask);
		/*cv::imshow("_debug_fgMovingMask", fgMovingMask);


		cv::Mat bgMovingMask = white_8UC1 - fgMovingMask;
		bgMask = white_8UC1 - fgMask;
		cv::imshow("_debug_bgMask", bgMask);
		cv::imshow("_debug_bgMovingMask", bgMovingMask);*/


		// debug_end

		// use moving contours to mask moving objects 
	}
	else; // use current background mask to mask moving objects

	// Background model: static parts of foreground (fg_act_masked) + moving parts of previous background (bg_act_masked)
	m_frame_act.copyTo(fg_act_masked, bgMask); 
	m_bgModel_prev.copyTo(bg_act_masked, fgMask);
	bg_model_act = fg_act_masked | bg_act_masked;

	//cv::imshow("bgMaskDiff", bgMask);
	//cv::imshow("fg_act_masked", fg_act_masked);
	//cv::imshow("bg_act_masked", bg_act_masked);
	//cv::imshow("bg_model_act", bg_model_act);


	/* generate background frame with first order low pass filter */
	/* fading factor = number of consecutive frames a background pixel has assumed 70% of step changed foreground value*/ 
	// NO mask to moving objects applied (generates ghosts)
	//m_bgModel_act = m_bgModel_prev * m_fadingFactor/(m_fadingFactor+1) + m_frame_act /(m_fadingFactor+1);
	
	// mask to moving objects applied
	m_bgModel_act = m_bgModel_prev * m_fadingFactor/(m_fadingFactor+1) + bg_model_act  /(m_fadingFactor+1);
	
	m_bgModel_prev = m_bgModel_act;
	bgModel = m_bgModel_act.clone();
}