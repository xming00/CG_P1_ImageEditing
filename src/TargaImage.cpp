///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

// constants
const int           RED = 0;                // red channel
const int           GREEN = 1;                // green channel
const int           BLUE = 2;                // blue channel
const unsigned char BACKGROUND[3] = { 0, 0, 0 };      // background color



// Computes n choose s, efficiently
double Binomial(int n, int s)
{
	double        res;

	res = 1;
	for (int i = 1; i <= s; i++)
		res = (n - i + 1) * res / i;

	return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
	data = new unsigned char[width * height * 4];
	ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char* d)
{
	int i;

	width = w;
	height = h;
	data = new unsigned char[width * height * 4];

	for (i = 0; i < width * height * 4; i++)
		data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image)
{
	width = image.width;
	height = image.height;
	data = NULL;
	if (image.data != NULL) {
		data = new unsigned char[width * height * 4];
		memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
	if (data)
		delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
	unsigned char* rgb = new unsigned char[width * height * 3];
	int		    i, j;

	if (!data)
		return NULL;

	// Divide out the alpha
	for (i = 0; i < height; i++)
	{
		int in_offset = i * width * 4;
		int out_offset = i * width * 3;

		for (j = 0; j < width; j++)
		{
			RGBA_To_RGB(data + (in_offset + j * 4), rgb + (out_offset + j * 3));
		}
	}

	return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char* filename)
{
	TargaImage* out_image = Reverse_Rows();

	if (!out_image)
		return false;

	if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
	{
		cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
		return false;
	}

	delete out_image;

	return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char* filename)
{
	unsigned char* temp_data;
	TargaImage* temp_image;
	TargaImage* result;
	int		        width, height;

	if (!filename)
	{
		cout << "No filename given." << endl;
		return NULL;
	}// if

	temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
	if (!temp_data)
	{
		cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
		width = height = 0;
		return NULL;
	}
	temp_image = new TargaImage(width, height, temp_data);
	free(temp_data);

	result = temp_image->Reverse_Rows();

	delete temp_image;

	return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char* d = Get_RGBA(j, i, data);
			float t = 0.3 * d[RED] + 0.59 * d[GREEN] + 0.11 * d[BLUE];

			d[RED] = t;
			d[GREEN] = t;
			d[BLUE] = t;

		}

	}
	return false;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char* d = Get_RGBA(j, i, data);
			d[RED] >>= 5;
			d[RED] <<= 5;
			d[GREEN] >>= 5;
			d[GREEN] <<= 5;
			d[BLUE] >>= 6;
			d[BLUE] <<= 6;

		}

	}

	return true;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{

	// uniform quantity first
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			unsigned char* d = Get_RGBA(x, y, data);
			d[RED] >>= 3;
			d[RED] <<= 3;
			d[GREEN] >>= 3;
			d[GREEN] <<= 3;
			d[BLUE] >>= 3;
			d[BLUE] <<= 3;

		}

	}
	Color Color_Nodes[32][32][32];
	vector<Color> Top256;
	// set Color_Nodes (i, j, k) => (r, g, b)
	for (int i = 0; i < 32; i++)
	{
		for (int j = 0; j < 32; j++)
		{
			for (int k = 0; k < 32; k++)
			{

				Color_Nodes[i][j][k].r = i * 8;
				Color_Nodes[i][j][k].g = j * 8;
				Color_Nodes[i][j][k].b = k * 8;
			}
		}
	}

	// count image pixel color to Color_Nodes
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			unsigned char* d = Get_RGBA(x, y, data);
			Color_Nodes[d[RED] / 8][d[GREEN] / 8][d[BLUE] / 8].count++;
		}
	}

	// sort to top 256
	for (int i = 0; i < 256; i++)
	{
		int mix = -1;
		int choosed_Index[3] = { -1, -1, -1 }; // {i , j, k}
		for (int i = 0; i < 32; i++)
		{
			for (int j = 0; j < 32; j++)
			{
				for (int k = 0; k < 32; k++)
				{
					if (Color_Nodes[i][j][k].count > mix && !Color_Nodes[i][j][k].beTake)
					{
						mix = Color_Nodes[i][j][k].count;
						choosed_Index[0] = i;
						choosed_Index[1] = j;
						choosed_Index[2] = k;
					}
				}
			}

		}
		Color_Nodes[choosed_Index[0]][choosed_Index[1]][choosed_Index[2]].beTake = true;
		Top256.push_back(Color_Nodes[choosed_Index[0]][choosed_Index[1]][choosed_Index[2]]);
	}
	//cout << "Top 256:\n";
	//for (int i = 0; i < 256; i++)
	//{
	//    cout << "Color Index(R,G,B): (" << (int)Top256[i].r << ", " << (int)Top256[i].g << ", " << (int)Top256[i].b << ")\n";
	//}

	// make difference (choose similar color)
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{

			unsigned char* d = Get_RGBA(x, y, data);
			Color Colsest_Color = Find_Closest_Palette_Color(d, Top256);
			d[RED] = Colsest_Color.r;
			d[GREEN] = Colsest_Color.g;
			d[BLUE] = Colsest_Color.b;
		}
	}


	return true;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char* d = Get_RGBA(j, i, data);
			double t = 0.30 * d[RED] + 0.59 * d[GREEN] + 0.11 * d[BLUE];
			if ((t / 255.0) >= 0.5)
			{
				d[RED] = 255;
				d[GREEN] = 255;
				d[BLUE] = 255;

			}
			else
			{
				d[RED] = 0;
				d[GREEN] = 0;
				d[BLUE] = 0;

			}


		}

	}
	return true;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
	// random
	srand(time(NULL));
	double randNum = (double)rand() / ((double)RAND_MAX) * 0.4 - 0.2;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			double randNum = (double)rand() / ((double)RAND_MAX) * 0.4 - 0.2;

			unsigned char* d = Get_RGBA(j, i, data);
			// add noise
			double t = 0.30 * d[RED] + 0.59 * d[GREEN] + 0.11 * d[BLUE];
			if ((t / 256.0 + randNum) >= 0.5)
			{
				d[RED] = 255;
				d[GREEN] = 255;
				d[BLUE] = 255;

			}
			else
			{
				d[RED] = 0;
				d[GREEN] = 0;
				d[BLUE] = 0;

			}


		}

	}
	return true;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
	To_Grayscale();

	// zig-zag way then find closest color
	for (int y = 0; y < height; y++)
	{
		// left to right
		if (y % 2 == 0)
		{
			int FS_Dir[4][2] = { {1, 1},{-1, 1},{0, 1},{1, 0} };     // (x,y)

			for (int x = 0; x < width; x++)
			{
				//cout << "value:" << x + y * width << endl;
				unsigned char* d = Get_RGBA(x, y, data);
				double t = d[RED];
				double err = t;
				if (t / 256.0f >= 0.5)
				{
					d[RED] = 255;
					d[GREEN] = 255;
					d[BLUE] = 255;
					err -= 255;
				}
				else
				{

					d[RED] = 0;
					d[GREEN] = 0;
					d[BLUE] = 0;
				}
				// add noise
				for (int now_Dir = 0; now_Dir < 4; now_Dir++)
				{
					if (Boundry_Check(x + FS_Dir[now_Dir][0], y + FS_Dir[now_Dir][1]))
					{
						unsigned char* be_Add_D = Get_RGBA(x + FS_Dir[now_Dir][0], y + FS_Dir[now_Dir][1], data);
						if (be_Add_D[RED] + (err * (double)(now_Dir * 2 + 1) / 16.0f) > 255)
							be_Add_D[RED] = 255;
						else if (be_Add_D[RED] + (err * (double)(now_Dir * 2 + 1) / 16.0f) < 0)
							be_Add_D[RED] = 0;
						else
							be_Add_D[RED] = be_Add_D[RED] + (err * (double)(now_Dir * 2 + 1) / 16.0f);

						if (be_Add_D[GREEN] + (err * (double)(now_Dir * 2 + 1) / 16.0f) > 255)
							be_Add_D[GREEN] = 255;
						else if (be_Add_D[GREEN] + (err * (double)(now_Dir * 2 + 1) / 16.0f) < 0)
							be_Add_D[GREEN] = 0;
						else
							be_Add_D[GREEN] = be_Add_D[GREEN] + (err * (double)(now_Dir * 2 + 1) / 16.0f);

						if (be_Add_D[BLUE] + (err * (double)(now_Dir * 2 + 1) / 16.0f) > 255)
							be_Add_D[BLUE] = 255;
						else if (be_Add_D[BLUE] + (err * (double)(now_Dir * 2 + 1) / 16.0f) < 0)
							be_Add_D[BLUE] = 0;
						else
							be_Add_D[BLUE] = be_Add_D[BLUE] + (err * (double)(now_Dir * 2 + 1) / 16.0f);


					}
				}
				//cout << "err:" << err << endl;
			}

		}
		else  // right to left
		{
			int FS_Dir[4][2] = { {-1, 1}, {1, 1}, {0, 1},{-1, 0} };     // (x,y)

			for (int x = width - 1; x >= 0; x--)
			{
				//cout << "value:" << x + y * width << endl;
				unsigned char* d = Get_RGBA(x, y, data);

				double t = d[RED];
				double err = t;
				if (t / 256.0f >= 0.5)
				{
					d[RED] = 255;
					d[GREEN] = 255;
					d[BLUE] = 255;
					err -= 255;
				}
				else
				{

					d[RED] = 0;
					d[GREEN] = 0;
					d[BLUE] = 0;
				}
				// add noise
				for (int now_Dir = 0; now_Dir < 4; now_Dir++)
				{
					if (Boundry_Check(x + FS_Dir[now_Dir][0], y + FS_Dir[now_Dir][1]))
					{
						unsigned char* be_Add_D = Get_RGBA(x + FS_Dir[now_Dir][0], y + FS_Dir[now_Dir][1], data);
						if (be_Add_D[RED] + (err * (double)(now_Dir * 2 + 1) / 16.0f) > 255)
							be_Add_D[RED] = 255;
						else if (be_Add_D[RED] + (err * (double)(now_Dir * 2 + 1) / 16.0f) < 0)
							be_Add_D[RED] = 0;
						else
							be_Add_D[RED] = be_Add_D[RED] + (err * (double)(now_Dir * 2 + 1) / 16.0f);

						if (be_Add_D[GREEN] + (err * (double)(now_Dir * 2 + 1) / 16.0f) > 255)
							be_Add_D[GREEN] = 255;
						else if (be_Add_D[GREEN] + (err * (double)(now_Dir * 2 + 1) / 16.0f) < 0)
							be_Add_D[GREEN] = 0;
						else
							be_Add_D[GREEN] = be_Add_D[GREEN] + (err * (double)(now_Dir * 2 + 1) / 16.0f);

						if (be_Add_D[BLUE] + (err * (double)(now_Dir * 2 + 1) / 16.0f) > 255)
							be_Add_D[BLUE] = 255;
						else if (be_Add_D[BLUE] + (err * (double)(now_Dir * 2 + 1) / 16.0f) < 0)
							be_Add_D[BLUE] = 0;
						else
							be_Add_D[BLUE] = be_Add_D[BLUE] + (err * (double)(now_Dir * 2 + 1) / 16.0f);

					}
				}
				//cout << "err:" << err << endl;

			}
		}
	}



	return true;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
	vector <double> tSort;
	double tSum = 0, tArv = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char* d = Get_RGBA(j, i, data);
			double t = 0.30 * d[RED] + 0.59 * d[GREEN] + 0.11 * d[BLUE];
			tSort.push_back(t / 256);
			tSum += (t / (double)256.0);
		}

	}
	sort(tSort.begin(), tSort.end());
	tArv = tSum / (double)(height * width);
	int  th = (1 - tArv) * height * width;
	double threshold = tSort[th];
	cout << "arv:" << threshold << endl;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char* d = Get_RGBA(j, i, data);
			double t = 0.30 * d[RED] + 0.59 * d[GREEN] + 0.11 * d[BLUE];
			if ((t / 256.0) >= threshold)
			{
				d[RED] = 255;
				d[GREEN] = 255;
				d[BLUE] = 255;

			}
			else
			{
				d[RED] = 0;
				d[GREEN] = 0;
				d[BLUE] = 0;

			}


		}

	}

	return true;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
	double mask[][4] = { {0.7059, 0.3529, 0.5882, 0.2353},
		{0.0588, 0.9412, 0.8235, 0.4118},
		{0.4706, 0.7647, 0.8824, 0.1176 },
		{0.1765, 0.5294, 0.2941, 0.6471 }
	};
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char* d = Get_RGBA(j, i, data);
			double t = 0.30 * d[RED] + 0.59 * d[GREEN] + 0.11 * d[BLUE];
			if ((t / 255.0) >= mask[i % 4][j % 4])
			{
				d[RED] = 255;
				d[GREEN] = 255;
				d[BLUE] = 255;

			}
			else
			{
				d[RED] = 0;
				d[GREEN] = 0;
				d[BLUE] = 0;

			}


		}

	}


	return true;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
	// zig-zag way then find closest color

	for (int y = 0; y < height; y++)
	{
		// left to right
		if (y % 2 == 0)
		{
			int FS_Dir[4][2] = { {1, 1},{-1, 1},{0, 1},{1, 0} };     // (x,y)

			for (int x = 0; x < width; x++)
			{
				unsigned char* d = Get_RGBA(x, y, data);
				unsigned char old[3] = { d[RED], d[GREEN], d[BLUE] };
				unsigned char err[3] = { 0 };
				d[RED] >>= 5;
				d[RED] <<= 5;
				d[GREEN] >>= 5;
				d[GREEN] <<= 5;
				d[BLUE] >>= 6;
				d[BLUE] <<= 6;
				for (int i = 0; i < 3; i++)
				{
					err[i] = abs(d[i] - old[i]);
				}

				for (int dir = 0; dir < 4; dir++)
				{
					if (Boundry_Check(x + FS_Dir[dir][0], y + FS_Dir[dir][1]))
					{
						unsigned char* Be_Add_D = Get_RGBA(x + FS_Dir[dir][0], y + FS_Dir[dir][1], data);
						if (Be_Add_D[RED] + err[RED] * (dir * 2 + 1) / 16.0f >= 255)
							Be_Add_D[RED] = 255;
						else if (Be_Add_D[RED] + err[RED] * (dir * 2 + 1) / 16.0f < 0)
							Be_Add_D[RED] = 0;
						else
						{
							Be_Add_D[RED] = Be_Add_D[RED] + err[RED] * (dir * 2 + 1) / 16.0f;
						}
						if (Be_Add_D[GREEN] + err[GREEN] * (dir * 2 + 1) / 16.0f >= 255)
							Be_Add_D[GREEN] = 255;
						else if (Be_Add_D[GREEN] + err[GREEN] * (dir * 2 + 1) / 16.0f < 0)
							Be_Add_D[GREEN] = 0;
						else
						{
							Be_Add_D[GREEN] = Be_Add_D[GREEN] + err[GREEN] * (dir * 2 + 1) / 16.0f;
						}
						if (Be_Add_D[BLUE] + err[BLUE] * (dir * 2 + 1) / 16.0f >= 255)
							Be_Add_D[BLUE] = 255;
						else if (Be_Add_D[BLUE] + err[BLUE] * (dir * 2 + 1) / 16.0f < 0)
							Be_Add_D[BLUE] = 0;
						else
						{
							Be_Add_D[BLUE] = Be_Add_D[BLUE] + err[BLUE] * (dir * 2 + 1) / 16.0f;
						}

					}

				}
			}

		}
		// right to left
		else
		{
			int FS_Dir[4][2] = { {-1, 1}, {1, 1}, {0, 1},{-1, 0} };     // (x,y)

			for (int x = width - 1; x >= 0; x--)
			{
				unsigned char* d = Get_RGBA(x, y, data);
				unsigned char old[3] = { d[RED], d[GREEN], d[BLUE] };
				unsigned char err[3] = { 0 };
				d[RED] >>= 5;
				d[RED] <<= 5;
				d[GREEN] >>= 5;
				d[GREEN] <<= 5;
				d[BLUE] >>= 6;
				d[BLUE] <<= 6;
				for (int i = 0; i < 3; i++)
				{
					err[i] = abs(d[i] - old[i]);
				}

				for (int dir = 0; dir < 4; dir++)
				{
					if (Boundry_Check(x + FS_Dir[dir][0], y + FS_Dir[dir][1]))
					{
						unsigned char* Be_Add_D = Get_RGBA(x + FS_Dir[dir][0], y + FS_Dir[dir][1], data);
						if (Be_Add_D[RED] + err[RED] * (dir * 2 + 1) / 16.0f >= 255)
							Be_Add_D[RED] = 255;
						else if (Be_Add_D[RED] + err[RED] * (dir * 2 + 1) / 16.0f < 0)
							Be_Add_D[RED] = 0;
						else
						{
							Be_Add_D[RED] = Be_Add_D[RED] + err[RED] * (dir * 2 + 1) / 16.0f;
						}
						if (Be_Add_D[GREEN] + err[GREEN] * (dir * 2 + 1) / 16.0f >= 255)
							Be_Add_D[GREEN] = 255;
						else if (Be_Add_D[GREEN] + err[GREEN] * (dir * 2 + 1) / 16.0f < 0)
							Be_Add_D[GREEN] = 0;
						else
						{
							Be_Add_D[GREEN] = Be_Add_D[GREEN] + err[GREEN] * (dir * 2 + 1) / 16.0f;
						}
						if (Be_Add_D[BLUE] + err[BLUE] * (dir * 2 + 1) / 16.0f >= 255)
							Be_Add_D[BLUE] = 255;
						else if (Be_Add_D[BLUE] + err[BLUE] * (dir * 2 + 1) / 16.0f < 0)
							Be_Add_D[BLUE] = 0;
						else
						{
							Be_Add_D[BLUE] = Be_Add_D[BLUE] + err[BLUE] * (dir * 2 + 1) / 16.0f;
						}

					}

				}
			}
		}
	}

	return true;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Over: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_In: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Out: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Atop: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
	if (width != pImage->width || height != pImage->height)
	{
		cout << "Comp_Xor: Images not the same size\n";
		return false;
	}

	ClearToBlack();
	return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
	if (!pImage)
		return false;

	if (width != pImage->width || height != pImage->height)
	{
		cout << "Difference: Images not the same size\n";
		return false;
	}// if

	for (int i = 0; i < width * height * 4; i += 4)
	{
		unsigned char        rgb1[3];
		unsigned char        rgb2[3];

		RGBA_To_RGB(data + i, rgb1);
		RGBA_To_RGB(pImage->data + i, rgb2);

		data[i] = abs(rgb1[0] - rgb2[0]);
		data[i + 1] = abs(rgb1[1] - rgb2[1]);
		data[i + 2] = abs(rgb1[2] - rgb2[2]);
		data[i + 3] = 255;
	}

	return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box()
{
	int Box_mask[5][5];
	int sum[3] = { 0 };
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			Box_mask[i][j] = 1;
		}
	}
	//cout  << "box_mask[3][3] : " << Box_mask[3][3] << endl;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int i = 0; i < 3; i++)
			{
				sum[i] = 0;
			}
			for (int i = -2; i < 3; i++)
			{
				for (int j = -2; j < 3; j++)
				{
					if (y + i < 0 || y + i >= height)
					{
						continue;
					}
					if (x + j < 0 || x + j >= width)
					{
						continue;
					}
					unsigned char* d = Get_RGBA(x + j, y + i, data);
					sum[RED] += (d[RED] * Box_mask[2 + i][2 + j]);
					sum[GREEN] += (d[GREEN] * Box_mask[2 + i][2 + j]);
					sum[BLUE] += (d[BLUE] * Box_mask[2 + i][2 + j]);

				}

			}
			unsigned char* nowD = Get_RGBA(x, y, data);
			// cout << "sum(red): " << sum[RED] << endl;
			nowD[RED] = sum[RED] / 25;
			nowD[GREEN] = sum[GREEN] / 25;
			nowD[BLUE] = sum[BLUE] / 25;


		}


	}



	return true;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
	double Box_mask[5][5] = { {1, 2, 3, 2, 1},
						{2, 4, 6, 4, 2},
						{3, 6, 9, 6, 3},
						{2, 4, 6, 4, 2},
						{1, 2, 3, 2, 1}, };
	int sum[3] = { 0 };
	//cout  << "box_mask[3][3] : " << Box_mask[3][3] << endl;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int i = 0; i < 3; i++)
			{
				sum[i] = 0;
			}
			for (int i = -2; i < 3; i++)
			{
				for (int j = -2; j < 3; j++)
				{
					if (y + i < 0 || y + i >= height)
					{
						continue;
					}
					if (x + j < 0 || x + j >= width)
					{
						continue;
					}
					unsigned char* d = Get_RGBA(x + j, y + i, data);
					sum[RED] += (d[RED] * Box_mask[2 + i][2 + j]);
					sum[GREEN] += (d[GREEN] * Box_mask[2 + i][2 + j]);
					sum[BLUE] += (d[BLUE] * Box_mask[2 + i][2 + j]);

				}

			}
			unsigned char* nowD = Get_RGBA(x, y, data);
			// cout << "sum(red): " << sum[RED] << endl;
			nowD[RED] = sum[RED] / 81;
			nowD[GREEN] = sum[GREEN] / 81;
			nowD[BLUE] = sum[BLUE] / 81;


		}


	}

	return false;
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
	int Box_mask[5][5] = { {1, 4, 6, 4, 1},
						   {4, 16, 24, 16, 4},
						   {6, 24, 36, 24, 6},
						   {4, 16, 24, 16, 4},
						   {1, 4, 6, 4, 1}, };
	int sum[3] = { 0 };

	//cout  << "box_mask[3][3] : " << Box_mask[3][3] << endl;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int i = 0; i < 3; i++)
			{
				sum[i] = 0;
			}
			for (int i = -2; i < 3; i++)
			{
				for (int j = -2; j < 3; j++)
				{
					if (y + i < 0 || y + i >= height)
					{
						continue;
					}
					if (x + j < 0 || x + j >= width)
					{
						continue;
					}
					unsigned char* d = Get_RGBA(x + j, y + i, data);
					sum[RED] += (d[RED] * Box_mask[2 + i][2 + j]);
					sum[GREEN] += (d[GREEN] * Box_mask[2 + i][2 + j]);
					sum[BLUE] += (d[BLUE] * Box_mask[2 + i][2 + j]);

				}

			}
			unsigned char* nowD = Get_RGBA(x, y, data);
			// cout << "sum(red): " << sum[RED] << endl;
			nowD[RED] = sum[RED] / 256;
			nowD[GREEN] = sum[GREEN] / 256;
			nowD[BLUE] = sum[BLUE] / 256;


		}


	}

	return true;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N(unsigned int N)
{
	vector<int> firstLine;
	vector< vector <int> > mask;
	firstLine.assign(N, 0);
	mask.assign(N, firstLine);
	int sum[3] = { 0 };
	int maskSum = 0;
	int distance = N / 2; // 中間到最邊邊的距離
	for (int i = 0; i < N; i++)
	{
		firstLine[i] = Binomial(N - 1, i);
	}
	// build first colume and row
	for (int i = 0; i < N; i++)
	{
		mask[0][i] = firstLine[i];
		mask[i][0] = firstLine[i];
	}
	for (int i = 1; i < N; i++)
	{
		for (int j = 1; j < N; j++)
		{
			mask[i][j] = mask[0][j] * mask[i][0];
		}

	}
	// for count sum
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			maskSum += mask[i][j];
		}

	}
	cout << "mask sum : " << maskSum << endl;
	// look gauss mask
	/*cout << "gauss mask : \n";
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			cout << mask[i][j] << " ";
		}
		cout << endl;
	}*/
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int i = 0; i < 3; i++)
			{
				sum[i] = 0;
			}
			for (int i = -distance; i <= distance; i++)
			{
				for (int j = -distance; j <= distance; j++)
				{
					if (y + i < 0 || y + i >= height)
					{
						continue;
					}
					if (x + j < 0 || x + j >= width)
					{
						continue;
					}
					unsigned char* d = Get_RGBA(x + j, y + i, data);
					sum[RED] += (d[RED] * mask[distance + i][distance + j]);
					sum[GREEN] += (d[GREEN] * mask[distance + i][distance + j]);
					sum[BLUE] += (d[BLUE] * mask[distance + i][distance + j]);

				}

			}
			unsigned char* nowD = Get_RGBA(x, y, data);
			// cout << "sum(red): " << sum[RED] << endl;
			nowD[RED] = sum[RED] / maskSum;
			nowD[GREEN] = sum[GREEN] / maskSum;
			nowD[BLUE] = sum[BLUE] / maskSum;


		}


	}



	return true;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
	int BartlletMask[5][5] = { {1, 4, 6, 4, 1},
							   {4, 16, 24, 16, 4},
							   {6, 24, 36, 24, 6},
							   {4, 16, 24, 16, 4},
							   {1, 4, 6, 4, 1} };
	int BartlletSum = 0;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			BartlletSum += BartlletMask[i][j];
		}

	}
	int originMask[5][5] = { 0 };
	originMask[2][2] = 1;
	int highPassMask[5][5] = { 0 };
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			highPassMask[i][j] = BartlletSum * originMask[i][j] - BartlletMask[i][j];
		}

	}
	// check

	cout << "High pass mask : \n";
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			cout << highPassMask[i][j] << " ";
		}
		cout << endl;
	}

	int sum[3] = { 0 };
	for (int turn = 0; turn < 3; turn++)
	{
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				for (int i = 0; i < 3; i++)
				{
					sum[i] = 0;
				}
				for (int i = -2; i < 3; i++)
				{
					for (int j = -2; j < 3; j++)
					{
						if (y + i < 0 || y + i >= height)
						{
							continue;
						}
						if (x + j < 0 || x + j >= width)
						{
							continue;
						}
						unsigned char* d = Get_RGBA(x + j, y + i, data);
						sum[RED] += (d[RED] * highPassMask[2 + i][2 + j]);
						sum[GREEN] += (d[GREEN] * highPassMask[2 + i][2 + j]);
						sum[BLUE] += (d[BLUE] * highPassMask[2 + i][2 + j]);

					}

				}
				unsigned char* nowD = Get_RGBA(x, y, data);
				// cout << "sum(red): " << sum[RED] << endl;
				if (sum[RED] / BartlletSum > 255)
					nowD[RED] = 255;
				else if (sum[RED] / BartlletSum < 0)
					nowD[RED] = 0;
				else
					nowD[RED] = sum[RED] / BartlletSum;

				if (sum[GREEN] / BartlletSum > 255)
					nowD[GREEN] = 255;
				else if (sum[GREEN] / BartlletSum < 0)
					nowD[GREEN] = 0;
				else
					nowD[GREEN] = sum[GREEN] / BartlletSum;

				if (sum[BLUE] / BartlletSum > 255)
					nowD[BLUE] = 255;
				else if (sum[BLUE] / BartlletSum < 0)
					nowD[BLUE] = 0;
				else
					nowD[BLUE] = sum[BLUE] / BartlletSum;




			}


		}
	}

	return true;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
	int BartlletMask[5][5] = { {1, 4, 6, 4, 1},
							  {4, 16, 24, 16, 4},
							  {6, 24, 36, 24, 6},
							  {4, 16, 24, 16, 4},
							  {1, 4, 6, 4, 1} };
	int BartlletSum = 0;
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			BartlletSum += BartlletMask[i][j];
		}

	}
	int originMask[5][5] = { 0 };
	originMask[2][2] = 2;
	int highPassMask[5][5] = { 0 };
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			highPassMask[i][j] = BartlletSum * originMask[i][j] - BartlletMask[i][j];
		}

	}

	int sum[3] = { 0 };

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int i = 0; i < 3; i++)
			{
				sum[i] = 0;
			}
			for (int i = -2; i < 3; i++)
			{
				for (int j = -2; j < 3; j++)
				{
					if (y + i < 0 || y + i >= height)
					{
						continue;
					}
					if (x + j < 0 || x + j >= width)
					{
						continue;
					}
					unsigned char* d = Get_RGBA(x + j, y + i, data);
					sum[RED] +=  (d[RED] * highPassMask[2 + i][2 + j]);
					sum[GREEN] +=  (d[GREEN] * highPassMask[2 + i][2 + j]);
					sum[BLUE] += (d[BLUE] * highPassMask[2 + i][2 + j]);

				}

			}
			unsigned char* nowD = Get_RGBA(x, y, data);
			// cout << "sum(red): " << sum[RED] << endl;
			if (sum[RED] / BartlletSum > 255)
				nowD[RED] = 255;
			else if (sum[RED] / BartlletSum < 0)
				nowD[RED] = 0;
			else
				nowD[RED] = sum[RED] / BartlletSum;

			if (sum[GREEN] / BartlletSum > 255)
				nowD[GREEN] = 255;
			else if (sum[GREEN] / BartlletSum < 0)
				nowD[GREEN] = 0;
			else
				nowD[GREEN] = sum[GREEN] / BartlletSum;

			if (sum[BLUE] / BartlletSum > 255)
				nowD[BLUE] = 255;
			else if (sum[BLUE] / BartlletSum < 0)
				nowD[BLUE] = 0;
			else
				nowD[BLUE] = sum[BLUE] / BartlletSum;




		}


	}

	return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{
	ClearToBlack();
	return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
//      use filter to origin , then half size forward mapping
// 
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
	float mask[][3] = { {0.0625, 0.1250, 0.0625},
						{0.1250, 0.2500, 0.1250},
						{0.0625, 0.1250, 0.0625} };
	// DO  filter
	float sum[3] = { 0 };

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			fill_Float_Value(3, 0, sum);
			for (int i = -1; i <= 1; i++)
			{
				for (int j = -1; j <= 1; j++)
				{
					// boundry case
					int correction[2] = { 0 };
					if (y + i < 0 || y + i >= height)
					{
						correction[1] = -2 * i; // mirror
					}
					if (x + j < 0 || x + j >= width)
					{
						correction[0] = -2 * j; // mirror
					}
					// normal case
					unsigned char* d = Get_RGBA(x + j + correction[0], y + i + correction[1], data);
					sum[RED] += (d[RED] * mask[1 + i][1 + j]);
					sum[GREEN] += (d[GREEN] * mask[1 + i][1 + j]);
					sum[BLUE] += (d[BLUE] * mask[1 + i][1 + j]);
				}
			}
			unsigned char* nowD = Get_RGBA(x, y, data);
			nowD[RED] = sum[RED];
			nowD[GREEN] = sum[GREEN];
			nowD[BLUE] = sum[BLUE];
		}

	}
	// delete data , then make new data (need temp data to catch old data
	unsigned char* temp_Data = new unsigned char[width * height * 4];
	memcpy(temp_Data, data, width * height * 4);
	delete[]data;
	data = new unsigned char[width / 2 * height / 2 * 4];
	memset(data, 0, (width / 2) * (height / 2) * 4);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			// f(u, v) =  (0.5u, 0.5v) = (x, y)  
			// &D[x * 4 + y * width * 4]

			// have a exception( when  )
			if (y / 2 == height / 2 || x / 2 == width / 2)
			{
				continue;
			}

			unsigned char* Data_RGBA = &data[int(x / 2) * 4 + int(y / 2) * int(width / 2) * 4];
			unsigned char* TData_RGBA = Get_RGBA(x, y, temp_Data);
			Data_RGBA[RED] = TData_RGBA[RED];
			Data_RGBA[GREEN] = TData_RGBA[GREEN];
			Data_RGBA[BLUE] = TData_RGBA[BLUE];
			Data_RGBA[3] = TData_RGBA[3];


		}

	}
	width /= 2;
	height /= 2;
	// change widget size


	delete[]temp_Data;


	return false;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
	ClearToBlack();
	return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
	ClearToBlack();
	return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
	ClearToBlack();
	return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char* rgba, unsigned char* rgb)
{
	const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

	unsigned char  alpha = rgba[3];

	if (alpha == 0)
	{
		rgb[0] = BACKGROUND[0];
		rgb[1] = BACKGROUND[1];
		rgb[2] = BACKGROUND[2];
	}
	else
	{
		float	alpha_scale = (float)255 / (float)alpha;
		int	val;
		int	i;

		for (i = 0; i < 3; i++)
		{
			val = (int)floor(rgba[i] * alpha_scale);
			if (val < 0)
				rgb[i] = 0;
			else if (val > 255)
				rgb[i] = 255;
			else
				rgb[i] = val;
		}
	}
}// RGA_To_RGB

//////////////////////////////////////////////////////////////////////////////
//
//      Help me to get RBGA data
//      
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::Get_RGBA(int x, int y, unsigned char* D)
{
	unsigned char* pos = &D[x * 4 + y * width * 4];
	return pos;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
	unsigned char* dest = new unsigned char[width * height * 4];
	TargaImage* result;
	int 	        i, j;

	if (!data)
		return NULL;

	for (i = 0; i < height; i++)
	{
		int in_offset = (height - i - 1) * width * 4;
		int out_offset = i * width * 4;

		for (j = 0; j < width; j++)
		{
			dest[out_offset + j * 4] = data[in_offset + j * 4];
			dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
			dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
			dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
		}
	}

	result = new TargaImage(width, height, dest);
	delete[] dest;
	return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
	memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//     for convenient to filter
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::fill_Float_Value(int num, float value, float* arr)
{
	for (int i = 0; i < num; i++)
	{
		arr[i] = value;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//     euclid distance 
//
///////////////////////////////////////////////////////////////////////////////

double TargaImage::euDistance(int nowX, int nowY, int nowZ, int x, int y, int z)
{
	return pow(pow(nowX - x, 2) + pow(nowY - y, 2) + pow(nowZ - z, 2), 0.5);
}

///////////////////////////////////////////////////////////////////////////////
//
//     Find_Closest_Palette_Color
//
///////////////////////////////////////////////////////////////////////////////

Color TargaImage::Find_Closest_Palette_Color(unsigned char* d, vector<Color> Top256)
{
	double min = 999999;
	int best_Color = -1;
	for (int i = 0; i < 256; i++)
	{
		if (euDistance(d[RED], d[GREEN], d[BLUE], Top256[i].r, Top256[i].g, Top256[i].b) < min)
		{
			min = euDistance(d[RED], d[GREEN], d[BLUE], Top256[i].r, Top256[i].g, Top256[i].b);
			best_Color = i;
		}
	}
	return Top256[best_Color];


}

///////////////////////////////////////////////////////////////////////////////
//
//      for boundry check 
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Boundry_Check(int x, int y)
{
	bool varify = true;
	if (x < 0 || x >= width)
	{
		varify = false;
	}
	if (y < 0 || y >= height)
	{
		varify = false;
	}
	return varify;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
	int radius_squared = (int)s.radius * (int)s.radius;
	for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
		for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
			int x_loc = (int)s.x + x_off;
			int y_loc = (int)s.y + y_off;
			// are we inside the circle, and inside the image?
			if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
				int dist_squared = x_off * x_off + y_off * y_off;
				if (dist_squared <= radius_squared) {
					data[(y_loc * width + x_loc) * 4 + 0] = s.r;
					data[(y_loc * width + x_loc) * 4 + 1] = s.g;
					data[(y_loc * width + x_loc) * 4 + 2] = s.b;
					data[(y_loc * width + x_loc) * 4 + 3] = s.a;
				}
				else if (dist_squared == radius_squared + 1) {
					data[(y_loc * width + x_loc) * 4 + 0] =
						(data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
					data[(y_loc * width + x_loc) * 4 + 1] =
						(data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
					data[(y_loc * width + x_loc) * 4 + 2] =
						(data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
					data[(y_loc * width + x_loc) * 4 + 3] =
						(data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
	unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
	radius(iradius), x(ix), y(iy), r(ir), g(ig), b(ib), a(ia)
{
}

