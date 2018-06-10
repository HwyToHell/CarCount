#include "stdafx.h"

#include "../include/frame_handler.h"



//////////////////////////////////////////////////////////////////////////////
// Arrow /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Arrow::Arrow(cv::Point start, cv::Point end, cv::Scalar color, int thickness):
	m_start(start),
	m_end(end),
	m_color(color),
	m_thickness(thickness) {
}


void Arrow::put(cv::Mat& image) {
	arrowedLine(image, m_start, m_end, m_color, m_thickness, 8, 0, 0.05);
	return;
}


//////////////////////////////////////////////////////////////////////////////
// Font /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Font::Font():
	face(cv::FONT_HERSHEY_SIMPLEX), 
	scale(0.5),
	color(white),
	thickness(1) {
}


//////////////////////////////////////////////////////////////////////////////
// TextRow ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
TextRow::TextRow(const int orgY, const Font font):
	m_font(font), m_origin(cv::Point(0,orgY)) {
}


/// Get text width in pixels
/// \return Width of text in pixels
int TextRow::getWidth() {
	int baseLine;
	cv::Size textSize = cv::getTextSize(m_text, m_font.face, m_font.scale,
		m_font.thickness, &baseLine);
	return textSize.width;
}


/// Draw actual text of row on image
/// \param[in] image Image to draw on
/// \param[in] colOrigin Column origin in pixels
void TextRow::put(cv::Mat& image, cv::Point colOrigin) {
	cv::Point rowOrigin = colOrigin + m_origin;
	cv::putText(image, m_text, rowOrigin, m_font.face, m_font.scale, m_font.color, m_font.thickness);
	return;
}

/// Set horizontal indentation according to row alignment (left, right)
/// \param[in] rowAlign (left, right)
/// \param[in] colWidth Column width in pixels
void TextRow::setIndent(Align rowAlign, int maxTextWidth) {

	// calc text width for row
	int baseLine;
	cv::Size textSize = cv::getTextSize(m_text, m_font.face, m_font.scale,
			m_font.thickness, &baseLine);

	// calc origin depending on alignment
	if (rowAlign == Align::left) {
		m_origin.x = 0;
	} else { // right align
		m_origin.x = maxTextWidth - textSize.width;
	}
}


/// Set text, if text width fits into available column width
/// otherwise fill available space with '#'
/// \param[in] text Text to set
/// \param[in] colWidth Column width in pixels
/// \return Width of text in pixels
int TextRow::setText(const std::string& text, const int colWidth) {
	int baseLine;
	cv::Size textSize = cv::getTextSize(text, m_font.face, m_font.scale,
		m_font.thickness, &baseLine);
	
	// fill with '#', if text too wide
	if (textSize.width < colWidth) {
		m_text = text;
	} else { // text too wide
		std::string textTooLong;
		cv::Size oneChar = cv::getTextSize("#", m_font.face, m_font.scale,
			m_font.thickness, &baseLine);
		int numChar = colWidth / oneChar.width;
		for (int n = 0; n < numChar; ++n) 
			textTooLong.push_back('#');
		textSize = cv::getTextSize(textTooLong, m_font.face, m_font.scale,
			m_font.thickness, &baseLine);
		m_text = textTooLong;
	}
	return textSize.width;
}



//////////////////////////////////////////////////////////////////////////////
// TextColumn ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
TextColumn::TextColumn(const cv::Point origin, const int colWidth, const Align colAlign):
	m_origin(origin), m_colWidth(colWidth), m_colAlign(colAlign) {}


/// Add text row to column block
/// \param[in] originY Vertical position of row text in pixels
/// \param[in] fon Font structure for row text
/// \return row index of added row
int TextColumn::addRow(const int originY, const Font font) {
	m_rowArray.push_back(TextRow(originY, font));
	int index = m_rowArray.size() - 1;
	return index;
}


// TextColumn::alignRows() functor for getting maximum width of all rows
class MaxRowWidth {
public:
	MaxRowWidth(): m_maxWidth(0) {};
	void operator() (TextRow& textRow) {
		if (textRow.getWidth() > m_maxWidth)
			m_maxWidth =  textRow.getWidth();
	}
	int	getWidth() {
		return m_maxWidth;
	}
private:
	int		m_maxWidth;
};


// TextColumn::alignRows() functor for setting row indentation
class SetRowIndent {
public:
	SetRowIndent(Align align, int textWidth): 
			m_align(align),
			m_textWidth(textWidth)
			{};
	void operator() (TextRow& textRow) {
		textRow.setIndent(m_align, m_textWidth);
	}
private:
	Align	m_align;
	int		m_textWidth;
};


/// Align text column block
/// \param[in] rowAlign Alignment of single text row (left, right)
/// \param[in] colAlign Alignment of embracing rectangle of text colum block (left, right)
/// \return True, if all texfields can be displayed in full length
void TextColumn::alignRows(Align rowAlign) {

	// get max text width in pixels for all rows
	MaxRowWidth maxRowWidth = std::for_each(m_rowArray.begin(), m_rowArray.end(), MaxRowWidth() );
	m_maxTextWidth = maxRowWidth.getWidth();

	// set row alignment for each row
	std::for_each(m_rowArray.begin(), m_rowArray.end(), SetRowIndent(rowAlign, m_maxTextWidth) );
	return;
}


/// Remove row
/// \param[in] rowIdx Row index
/// \return True, if row was successfully removed
bool TextColumn::removeRow(const int rowIdx) {
	size_t idx = static_cast<size_t>(rowIdx);
	if (idx >= m_rowArray.size() ) {
		std::cerr << "removeRow: row index: " << idx << " out of array size: " 
			<< m_rowArray.size() << std::endl;
		return false;
	} else { // index within bounds
		m_rowArray.erase(m_rowArray.begin() + rowIdx);
		return true;
	}
}


/// Draw text of all rows in column on image
/// column block left or right aligned
/// \param[in] image Image to draw on
void TextColumn::put(cv::Mat& image) {
	
	// no extra indent for left aligned
	cv::Point offset(0,0);
	
	// extra indent for right aligned column
	if (m_colAlign == Align::right)
		offset.x = m_colWidth - m_maxTextWidth;

	// show each row with offset applied
	RowArray::iterator iRow = m_rowArray.begin();
	while (iRow != m_rowArray.end()) {
		iRow->put(image, m_origin + offset);
		++iRow;
	}
}


/// Resize text column
/// \param[in] origin Origin of text column
/// \param[in] width Width of text column
/// \return True, if text for given row was successfully set
void TextColumn::resize(const cv::Point origin, const int colWidth) {
	m_origin = origin;
	m_colWidth = colWidth;
	return;
}


/// Set text for row
/// \param[in] rowIdx Row index
/// \param[in] text Text to set
/// \return True, if text for given row was successfully set
bool TextColumn::setRowText(const int rowIdx, const std::string& text) {
	size_t idx = static_cast<size_t>(rowIdx);
	if (idx >= m_rowArray.size() ) {
		std::cerr << "setRowText: row index: " << idx << " out of array size: " 
			<< m_rowArray.size() << std::endl;
		return false;
	} else { // index within bounds
		m_rowArray.at(rowIdx).setText(text, m_colWidth);
		return true;
	}
}



//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/// load inset from file
/// resize based on frame width
bool loadInset(std::string insetFilePath, cv::Size frameSize, cv::Mat& outInset) {
	using namespace std;

	// load inset image to display counting results on
	if (!isFileExist(insetFilePath)) {
		cerr << "loadInset: path to inset image does not exist: " << insetFilePath << endl;
	}

	cv::Mat inset_org = cv::imread(insetFilePath);
	if (inset_org.data) { 
		// use loaded image, fit to current frame size
		cv::Size insetSize = inset_org.size();
		double scaling = frameSize.width / (double)insetSize.width;
		cv::resize(inset_org, outInset, cv::Size(), scaling, scaling);
	} else { // create black matrix with same dimensions
		cerr << "loadInset: could not open " << insetFilePath << ", use substitute inset" << endl;
		int width = (int)frameSize.width;
		int height = 65;
		cv::Mat insetSubst(height, width, CV_8UC3, black);
		outInset = insetSubst;
	}
	return true;
}


/// create arrow pointing to left or right
/// based on inset origin
Arrow createArrow(const cv::Size inset, const Align pointsTo) {
	
	// layout
	// left border | arrow to left | divider | arrow to right | right border
	int length = inset.width * 10 / 24;
	int xBorder = inset.width * 1 / 24;
	int yBorder = inset.height * 3 / 24;

	// appearance
	int thickness = (inset.width > 200) ? (inset.width / 200) : 1;

	// arrow coords depending on points to
	cv::Point start(0,0), end(0,0);
	if (pointsTo == Align::left) {	// arrow to left
		start = cv::Point(xBorder + length, yBorder);
		end = cv::Point(xBorder, yBorder);
	} else {						// arrow to right
		start = cv::Point(inset.width - xBorder - length, yBorder);
		end = cv::Point(inset.width - xBorder, yBorder);
	}

	return Arrow(start, end, green, thickness);
}


/// create left or right column based on inset size
/// text column origin relative to inset origin
TextColumn createTextColumn(const cv::Size inset, const  Align colAlign) {
	
	// layout:
	// left border | image column | text column | divider | text column | image column | right border
	int widthImageCol = inset.width * 3 / 24;
	int widthTextCol = inset.width * 8 / 24;

	// yOrigin = 0 -> top of inset
	int yOrigin = 0;

	// xOrigin based on alignment
	int xOrigin = 0;
	if (colAlign == Align::right)
		xOrigin = inset.width - widthImageCol - widthTextCol;
	else
		xOrigin = widthImageCol;

	// create temp column as ret val
	TextColumn textCol(cv::Point(xOrigin, yOrigin), widthTextCol, colAlign);

	// font for all rows
	Font font;
	font.face      = cv::FONT_HERSHEY_SIMPLEX;
	font.scale	   = static_cast<double>(inset.width) / 600;
	font.color	   = green;
	font.thickness = (inset.width > 200) ? (inset.width / 200) : 1;

	// calculate row origins from inset size
	int yFirstRow = inset.height * 11 / 24;
	int ySecondRow = inset.height * 20 / 24; 

	// insert rows
	textCol.addRow(yFirstRow, font);
	textCol.addRow(ySecondRow, font);

	return textCol;
}


/// copy inset into frame
void putInset(const cv::Mat& inset, cv::Mat& frame) {

	// show inset with vehicle icons and arrows
	if (inset.data) {
		// TODO adjust copy position depending on frame size
		int yInset = frame.rows - inset.rows; 
		inset.copyTo(frame(cv::Rect(0, yInset, inset.cols, inset.rows)));
	}
}





int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	//Size2i dispSize(160, 120);
	//Size2i dispSize(320, 240);
	//Size2i dispSize(480, 360);
	//Size2i dispSize(640, 480);
	Size2i dispSize(800, 600);
	string insetImage("D:\\Users\\Holger\\counter\\inset4.png");
	Mat frame(dispSize.height, dispSize.width, CV_8UC3, gray);
	Mat inset;

	loadInset(insetImage, dispSize, inset);

	// manipulate inset content
	//	 create text columns
	TextColumn textColLeft = createTextColumn(inset.size(), Align::left);
	TextColumn textColRight = createTextColumn(inset.size(), Align::right);
	
	//   create Arrow
	Arrow arrowLeft = createArrow(inset.size(), Align::left);
	Arrow arrowRight = createArrow(inset.size(), Align::right);

	//	 put arrow
	arrowLeft.put(inset);
	arrowRight.put(inset);

	//   set and put text
	textColLeft.setRowText(0, "1234567890");
	textColLeft.setRowText(1, "Left");
	textColLeft.alignRows(Align::right);
	textColLeft.put(inset);

	textColRight.setRowText(0, "12345678");
	textColRight.setRowText(1, "Right");
	textColRight.alignRows(Align::right);
	textColRight.put(inset);

	putInset(inset, frame);
	
	imshow("frame", frame);
	imshow("inset", inset);

	waitKey(0);

	
	/*cout << "Press <enter> to exit" << endl;
	string str;
	getline(std::cin, str);
	*/
	return 0;
}