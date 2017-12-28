	cv::Mat fTest(240, 320, CV_8UC3);
	cv::Mat rowDens(240, 1, CV_32SC1);
	cv::Mat colDens(1, 320, CV_32SC1);
		// Density per row and column
		reduce(fgMaskShadow, colDens, 0, CV_REDUCE_SUM, CV_32SC1); // reduced to row
		reduce(fgMaskShadow, rowDens, 1, CV_REDUCE_SUM, CV_32SC1); // reduced to col
		cvtColor(fgMaskShadow, fTest, CV_GRAY2RGB);
		// show summary density of rows and columns: 
		int ch = 3;
		// Rows
		for (int j = 0; j < fTest.rows; j++)
			for (int i = 0; i < rowDens.at<int>(j, 0) / 255; i++)
			{
				fTest.at<uchar>(j, i*ch) = 0;
				fTest.at<uchar>(j, i*ch+1) = 0;
				fTest.at<uchar>(j, i*ch+2) = 255;
			}

		// Columns
		for (int i = 0; i < fTest.cols; i++)
			for (int j = fTest.rows - colDens.at<int>(0, i) / 255; j < fTest.rows; j++)
			{
				if ( (colDens.at<int>(0, i) / 255) > 5)
				{
					fTest.at<uchar>(j, i*ch) = 0;
					fTest.at<uchar>(j, i*ch+1) = 0;
					fTest.at<uchar>(j, i*ch+2) = 255;
				}
				else
				{
					fTest.at<uchar>(j, i*ch) = 255;
					fTest.at<uchar>(j, i*ch+1) = 0;
					fTest.at<uchar>(j, i*ch+2) = 0;
				}
			}