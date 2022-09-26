#include "bmpreader.h"

BmpHandler::BmpHandler(string _path_) : path(_path_)
{
    this->img = nullptr;
    this->isGrayScale = false;
    if (this->path != "")
    {
        this->loadBmpImage();
    }
}

int BmpHandler::getImgHeight() const
{
    return this->img_dim.first;
}

int BmpHandler::getImgWidth() const
{
    return this->img_dim.second;
}

void BmpHandler::applyAutoContrast()
{
    if (this->img)
    {
        for (unsigned int row = 0; row < this->img_dim.first; row++)
        {
            for (unsigned int col = 0; col < this->img_dim.second; ++col)
            {
                this->img[0][row][col] = (this->img[0][row][col] - this->contrast[0][1]) * (255 / (this->contrast[0][0] - this->contrast[0][1]));
                this->img[1][row][col] = (this->img[1][row][col] - this->contrast[1][1]) * (255 / (this->contrast[1][0] - this->contrast[1][1]));
                this->img[2][row][col] = (this->img[2][row][col] - this->contrast[2][1]) * (255 / (this->contrast[2][0] - this->contrast[2][1]));
            }
        }
    }
    else
    {
        cerr << "\nThere is no data in buffer {this->img}, AutoContrast cannot be applied...";
    }
}

void BmpHandler::applyGrayscale()
{
    if (this->img)
    {
        for (unsigned int row = 0; row < this->img_dim.first; ++row)
        {
            for (unsigned int col = 0; col < this->img_dim.second; ++col)
            {
                // 0.299 ∙ Red + 0.587 ∙ Green + 0.114 ∙ Blue (Another way to grayscale)
                // go_board.image[0][row][width] = go_board.image[1][row][width] =  go_board.image[2][row][width] = 0.299 * go_board.tmpbuf[2] + 0.587 * go_board.tmpbuf[1] + 0.114 * go_board.tmpbuf[0]
                this->img[0][row][col] = this->img[1][row][col] = this->img[2][row][col] = (this->img[0][row][col] + this->img[1][row][col] + this->img[2][row][col]) / 3;
            }
        }
        isGrayScale = true;
    }
    else
    {
        cerr << "\nThere is no data in buffer {this->img}, Grayscale cannot be applied...";
    }
}

int BmpHandler::abs(int val)
{
    return (val >= 0) ? val : (-1 * val);
}

vector<pair<int, int>> BmpHandler::getEdgePoint() const
{
    return this->edgePoints;
}

void BmpHandler::singleRowEdgeDetection(int row, bool displayEdges, int padding, int skip_amount)
{
    if (this->img)
    {
        if (this->isGrayScale)
        {
            if ((row >= 0) &&
                (row < this->img_dim.first))
            {
                pair<int, int> coord;
                for (unsigned int col = padding; col < (this->img_dim.second - padding); ++col)
                {
                    if (this->abs((int)this->img[0][row][col] - (int)this->img[0][row][col + 1]) >= 30)
                    {
                        // Store the coords in a vector
                        coord.first = row;
                        coord.second = col;
                        edgePoints.push_back(coord);

                        // Make the edges detected visible (white)
                        if (displayEdges)
                        {
                            this->createBorder(row - 1, col - 1, row + 1, col + 1);
                        }

                        // To make sure pixels aren't clumped together, we will skip an extra step
                        col += skip_amount;
                    }
                }
            }
            else
            {
                cerr << "\n The entered row in {this->singleRowEdgeDetection} is out of bounds...";
            }
        }
        else
        {
            cerr << "\n The image needs to be in grayscale before you can apply singleRowEdgeDetection on it";
        }
    }
    else
    {
        cerr << "\n There was no data in buffer {this->img}, singleRowEdgeDetection cannot be applied...";
    }
}

void BmpHandler::loadBmpImage()
{
    if (this->path != "") // Check if path is set
    {
        fstream bmp;
        bmp.open(path + ".bmp", ios::in);
        if (bmp)
        {
            this->img_dim = this->getImageDimensions(bmp); // This will load the Height x Width of the bmp
            this->allocateBuffer(this->img);               // This will allocate the memory for the 3D array holding the image
            this->allocateBuffer(this->original_img);      // This will allocate the memory for the 3D array holding the original image
            readBMPImage(bmp);                             // This will read the bmp and load it into the data structure
        }
        bmp.close();
    }
    else
    {
        cerr << "\nThere was no <path> given from which bmp file could be loaded...";
    }
}

void BmpHandler::setPath(string _path_)
{

    if (_path_ == "")
    {
        cerr << "\nYou can't pass an empty char array as a path...";
    }
    else
    {
        this->path = _path_;
    }
}

BmpHandler::~BmpHandler()
{
    // Deallocation of original image
    if (this->img)
    {
        for (int i = 0; i < COLOR_CHANNELS; ++i)
        {
            for (int j = 0; j < this->img_dim.first; ++j)
            {
                delete[] this->img[i][j];
            }
            delete[] this->img[i];
        }
        delete[] this->img;
        this->img = nullptr;
    }

    if (this->original_img)
    {
        for (int i = 0; i < COLOR_CHANNELS; ++i)
        {
            for (int j = 0; j < this->img_dim.first; ++j)
            {
                delete[] this->original_img[i][j];
            }
            delete[] this->original_img[i];
        }
        delete[] this->original_img;
        this->original_img = nullptr;
    }
}

void BmpHandler::writeBMPImage(bool imgType)
{
    unsigned char ***buf;
    if (imgType)
    {
        buf = this->original_img;
    }
    else
    {
        buf = this->img;
    }

    if (buf)
    {
        fstream bmp;
        int avg = 0;
        bmp.open(this->path + "_w.bmp", ios::out);
        if (bmp)
        {
            bmp.write(this->header, HEADER_LEN);
            for (unsigned int row = 0; row < this->img_dim.first; ++row)
            {
                for (unsigned int col = 0; col < this->img_dim.second; ++col)
                {
                    bmp << (char)buf[0][row][col] << (char)buf[1][row][col] << (char)buf[2][row][col];
                }
            }
        }
        else
        {
            cerr << "\nThere was an error opening/creating your bmp image <" << path << "_w.bmp>";
        }
        bmp.close();
    }
    else
    {
        cerr << "\nThere was no data in buffer {this->img}, Nothing could be written in {this->path + \"_w.bmp\"}...";
    }

    buf = nullptr; // dereference the temp buffer
}

pair<uint16_t, uint16_t> BmpHandler::getImageDimensions(fstream &bmp)
{
    bmp.read(this->header, HEADER_LEN);

    uint16_t width = (uint8_t)this->header[19];
    width <<= 8;
    width += (uint8_t)this->header[18];

    uint16_t height = (uint8_t)this->header[23];
    height <<= 8;
    height += (uint8_t)this->header[22];

    pair<uint16_t, uint16_t> img_dim;
    img_dim.first = height;
    img_dim.second = width;

    return img_dim;
}

void BmpHandler::createBorder(int x1, int y1, int x2, int y2)
{
    if ((x1 <= x2) ||
        (y1 <= y2))
    {
        if ((x1 >= 0) &&
            (x2 < this->img_dim.first) &&
            (y1 >= 0) &&
            (y2 < this->img_dim.second))
        {
            // (x1,y1) to (x1,y2) Top Line
            for (unsigned int col = y1; col <= y2; ++col)
            {
                this->img[0][x1][col] = 255;
                this->img[1][x1][col] = 0;
                this->img[2][x1][col] = 0;
            }

            // (x2,y1) to (x2,y2) Bottom Line
            for (unsigned int col = y1; col <= y2; ++col)
            {
                this->img[0][x2][col] = 255;
                this->img[1][x2][col] = 0;
                this->img[2][x2][col] = 0;
            }

            // (x1,y1) to (x2,y1) Left Line
            for (unsigned int row = x1; row <= x2; ++row)
            {
                this->img[0][row][y1] = 255;
                this->img[1][row][y1] = 0;
                this->img[2][row][y1] = 0;
            }

            // (x1,y2) to (x2,y2) Right Line
            for (unsigned int row = x1; row <= x2; ++row)
            {
                this->img[0][row][y2] = 255;
                this->img[1][row][y2] = 0;
                this->img[2][row][y2] = 0;
            }
        }
        else
        {
            cerr << "\nThe coordinates are not within bounds of the image dimension...";
        }
    }
    else
    {
        cerr << "\nThe coordinates do not seem to be correctly placed...";
    }
}

int BmpHandler::pow(int val)
{
    return val * val;
}

void BmpHandler::singleRowTemplateMatching(BmpHandler &bmpB, int rowOffSet)
{
    if (this->edgePoints.size() > 0)
    {
        int best_ssd;
        int curr_ssd;
        pair<int, int> best_coord; // This will store the index (x,y) for the best ssd
        for (int edge_no = 0; edge_no < this->edgePoints.size(); edge_no++)
        {
            best_ssd = INT32_MAX;
            // Compare with all patches in a row of ImageB
            for (int col = 1; col < (this->edgePoints[edge_no].second - 1); col++)
            {
                // Compare and find SSD of each patch
                curr_ssd = 0;
                for (int c = 0; c < COLOR_CHANNELS; c++)
                {
                    for (int r = 0; r < 3; r++)
                    {
                        for (int w = 0; w < 3; w++)
                        {
                            curr_ssd += this->pow(this->abs(this->original_img[c]
                                                                              [(this->edgePoints[edge_no].first - 1) + r]
                                                                              [(this->edgePoints[edge_no].second - 1) + w] -
                                                            bmpB.original_img[c]
                                                                             [(this->edgePoints[edge_no].first + rowOffSet - 1) + r]
                                                                             [(col - 1) + w]));
                        }
                    }
                }

                // Compare curr_ssd with best_ssd
                if (curr_ssd < best_ssd)
                {
                    best_ssd = curr_ssd;
                    best_coord.first = this->edgePoints[edge_no].first + rowOffSet;
                    best_coord.second = col;
                }
            }

            // 5) Print the most correlating point in the second image
            bmpB.createBorder(best_coord.first - 1, best_coord.second - 1, best_coord.first + 1, best_coord.second + 1);
            cout << "\n"
                 << edgePoints[edge_no].first << "," << edgePoints[edge_no].second
                 << " = "
                 << best_coord.first << "," << best_coord.second;
        }
    }
    else
    {
        cerr << "\nThere were no edge points in {this->edgePoints}...";
    }
}

void BmpHandler::findMinMaxContrast(int blue, int red, int green)
{
    // Channel BLUE
    if (blue < this->contrast[0][1])
    {
        this->contrast[0][1] = blue;
    }
    if (blue > this->contrast[0][0])
    {
        this->contrast[0][0] = blue;
    }
    // Channel Green
    if (green < this->contrast[1][1])
    {
        this->contrast[1][1] = green;
    }
    if (green > this->contrast[1][0])
    {
        this->contrast[1][0] = green;
    }
    // Channel Red
    if (red < this->contrast[2][1])
    {
        this->contrast[2][1] = red;
    }
    if (red > this->contrast[2][0])
    {
        this->contrast[2][0] = red;
    }
}

void BmpHandler::readBMPImage(fstream &bmp)
{
    int row = 0;
    int width = 0;
    char tmp[3];
    while (row < this->img_dim.first)
    {
        width = 0;
        while (width < this->img_dim.second)
        {
            bmp.read(tmp, 3);
            for (unsigned int color = 0; color < COLOR_CHANNELS; ++color)
            {
                this->img[color][row][width] = tmp[color];
                this->original_img[color][row][width] = tmp[color];
            }
            findMinMaxContrast(this->img[0][row][width], this->img[1][row][width], this->img[2][row][width]);
            width++;
        }
        row++;
    }
}

void BmpHandler::allocateBuffer(unsigned char ***&buf)
{
    buf = new unsigned char **[COLOR_CHANNELS];
    for (unsigned int channel = 0; channel < COLOR_CHANNELS; ++channel)
    {
        buf[channel] = new unsigned char *[this->img_dim.first];
        for (unsigned int row = 0; row < this->img_dim.first; ++row)
        {
            buf[channel][row] = new unsigned char[this->img_dim.second];
        }
    }
}