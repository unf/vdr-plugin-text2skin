//								-*- c++ -*-

#include "bitmap.h"
#include "setup.h"
#include <vdr/tools.h>
#define X_DISPLAY_MISSING
#ifdef HAVE_IMAGEMAGICK
#include <Magick++.h>
using namespace Magick;
#endif
#include <glob.h>

cBitmapCache cText2SkinBitmap::mCache(Text2SkinSetup.MaxCacheFill);

void cBitmapCache::DeleteObject(const tBitmapSpec &Key, cText2SkinBitmap *&Data)
{
	delete Data;
}

void cBitmapCache::ResetObject(cText2SkinBitmap *&Data)
{
	Data->Reset();
}


cText2SkinBitmap *cText2SkinBitmap::Load(const std::string &Filename, int Alpha, int height,
                                         int width, int colors, bool Quiet, bool TrueColor) {
	// colour reduction only serves the palette of a 256 colour OSD
	if (TrueColor)
		colors = 0;
	tBitmapSpec spec(Filename, Alpha, height, width, colors, TrueColor);
	Dprintf("checking image with spec %s_%d_%d_%d_%d..", Filename.c_str(),Alpha,height,width,colors);
	std::string fname = Filename;

	cText2SkinBitmap *res = NULL;
	if (mCache.Contains(spec)) {
		res = mCache[spec];
		Dprintf("..cache ok\n");
	} else {
		if (fname.find('*') != std::string::npos) {
			glob_t gbuf;
			if (glob(fname.c_str(), 0, NULL, &gbuf) == 0){
				Dprintf("GLOB: FOUND %s\n", gbuf.gl_pathv[0]);
				fname = gbuf.gl_pathv[0];
			} else {
				if (!Quiet)
					esyslog("ERROR: text2skin: No match for wildcard filename %s",
							Filename.c_str());
				fname = "";
			}
			globfree(&gbuf);
		}

		res = new cText2SkinBitmap;
		int len = fname.length();
		bool result = false;
		if (len > 4) {
			if (fname.substr(len - 4, 4) == ".xpm")
				result = res->LoadXpm(fname.c_str());
			else {
				result = res->LoadNonXpm(fname.c_str(), height, width, colors, Quiet, TrueColor);
			}
		} else if (!Quiet)
			esyslog("ERROR: text2skin: filename %s too short to identify format", fname.c_str());

		Dprintf("..load %sok\n", result ? "" : "not ");
		if (result)
			res->SetAlpha(Alpha);
		else
			DELETENULL(res);
		mCache[spec] = res;
	}
	return res;
}

bool cText2SkinBitmap::Available(const std::string &Filename, int Alpha, int height, int width,
                                 int colors)
{
	if ((int)Filename.find('*') != -1) {
		bool result = false;
		glob_t gbuf;
		if (glob(Filename.c_str(), 0, NULL, &gbuf) == 0)
			result = true;
		globfree(&gbuf);
		return result;
	} else
		return access(Filename.c_str(), F_OK) == 0;
}

void cText2SkinBitmap::Init(void) {
#ifdef HAVE_IMAGEMAGICK
	InitializeMagick(NULL);
#endif
	mCache.SetMaxItems(Text2SkinSetup.MaxCacheFill);
}

cText2SkinBitmap::cText2SkinBitmap(void):
	mCurrent(0),
	mLastGet(0)
{
}

cText2SkinBitmap::~cText2SkinBitmap() {
	for (int i = 0; i < (int)mBitmaps.size(); ++i)
		delete mBitmaps[i];
	mBitmaps.clear();
	for (int i = 0; i < (int)mImages.size(); ++i)
		delete mImages[i];
	mImages.clear();
}

cBitmap &cText2SkinBitmap::Get(uint &UpdateIn, uint Now) {
	NextFrame(UpdateIn, Now);
	return *mBitmaps[mCurrent];
}

const cImage &cText2SkinBitmap::GetImage(uint &UpdateIn, uint Now) {
	NextFrame(UpdateIn, Now);
	return *mImages[mCurrent];
}

// animated images: advance mCurrent after the frame delay
void cText2SkinBitmap::NextFrame(uint &UpdateIn, uint Now) {
	int frames = IsImage() ? mImages.size() : mBitmaps.size();
	if (frames <= 1)
		return;

	time_t upd;
	int diff;
	if (mLastGet == 0) {
		mLastGet = Now;
		upd = mDelay;
	} else if ((diff = Now - mLastGet) >= mDelay) {
		mCurrent = (mCurrent + 1) % frames;
		mLastGet = Now;
		upd = mDelay - diff > 1 ? mDelay - diff : 1;
	} else {
		upd = mDelay - diff;
	}

	if (UpdateIn == 0 || UpdateIn > (uint)upd)
		UpdateIn = upd;
}

void cText2SkinBitmap::SetAlpha(int Alpha) {
	if (Alpha > 0) {
		for (int i = 0; i < (int)mImages.size(); ++i) {
			tColor *p = (tColor *)mImages[i]->Data();
			for (int n = mImages[i]->Width() * mImages[i]->Height(); n > 0; --n, ++p)
				*p = (*p & 0x00FFFFFF) | ((((*p >> 24) * Alpha / 255) & 0xFF) << 24);
		}
		std::vector<cBitmap*>::iterator it = mBitmaps.begin();
		for (; it != mBitmaps.end(); ++it) {
			int count;
			if ((*it)->Colors(count)) {
				for (int i = 0; i < count; ++i) {
					int alpha = (((*it)->Color(i) & 0xFF000000) >> 24) * Alpha / 255;
					(*it)->SetColor(i, ((*it)->Color(i) & 0x00FFFFFF) | (alpha << 24));
				}
			}
		}
	}
}

bool cText2SkinBitmap::LoadXpm(const char *Filename) {
	cBitmap *bmp = new cBitmap(1,1,1);
	if (bmp->LoadXpm(Filename)) {
		mBitmaps.push_back(bmp);
		return true;
	}
	delete bmp;
	return false;
}

bool cText2SkinBitmap::LoadNonXpm(const char *Filename, int height, int width, int colors, bool Quiet,
                                  bool TrueColor) {

#ifdef HAVE_IMAGEMAGICK
	std::vector<Image> images;
	cBitmap *bmp = NULL;
	cImage *img = NULL;
	try {
		int w, h;
		std::vector<Image>::iterator it;
		readImages(&images, Filename);
		if (images.empty()) {
			esyslog("ERROR: text2skin: Couldn't load %s", Filename);
			return false;
		}
		mDelay = images[0].animationDelay() * 10;
		if (images.size() > 1) {
			// frames of an optimized animation hold only what changed,
			// build complete frames before they are scaled one by one
			std::vector<Image> frames;
			coalesceImages(&frames, images.begin(), images.end());
			images.swap(frames);
		}
		for (it = images.begin(); it != images.end(); ++it) {
			// scale with a Lanczos filter instead of sample() (nearest neighbour,
			// blocky when enlarged), and before quantizing, so the filter
			// cannot add colours beyond the requested count. A palette without
			// colour reduction has no room for the blended colours of a filter,
			// the image would lose its colours, so it keeps sample() there.
			if (height != 0 || width != 0) {
				if (TrueColor || colors != 0) {
					(*it).filterType(LanczosFilter);
					(*it).zoom(Geometry(width,height));
				} else
					(*it).sample(Geometry(width,height));
			}
			if (colors != 0){
				(*it).opacity(OpaqueOpacity);
				(*it).backgroundColor( Color ( 0,0,0,0) );
				(*it).quantizeColorSpace( RGBColorspace );
				(*it).quantizeColors( colors );
				(*it).quantize();
			}
			w = (*it).columns();
			h = (*it).rows();
			const PixelPacket *pix = (*it).getConstPixels(0, 0, w, h);

			if (TrueColor) {
				// A Q16 build scales with 16 bits per channel. Cutting that to
				// 8 bits turns smooth gradients into visible bands, an ordered
				// dither keeps the steps in between. With Q8 nothing changes.
				static const int bayer[4][4] = {
					{  0,  8,  2, 10 }, { 12,  4, 14,  6 },
					{  3, 11,  1,  9 }, { 15,  7, 13,  5 } };
				const uint64_t scale = (uint64_t)MaxRGB * 16;
				img = new cImage(cSize(w, h));
				tColor *dst = (tColor *)img->Data();
				for (int iy = 0; iy < h; ++iy) {
					for (int ix = 0; ix < w; ++ix, ++pix) {
						uint64_t t = (2 * bayer[iy & 3][ix & 3] + 1) * (uint64_t)MaxRGB / 2;
						tColor r = ((uint64_t)pix->red   * 255 * 16 + t) / scale;
						tColor g = ((uint64_t)pix->green * 255 * 16 + t) / scale;
						tColor b = ((uint64_t)pix->blue  * 255 * 16 + t) / scale;
						tColor a = 255 - ((uint64_t)pix->opacity * 255 + MaxRGB / 2) / MaxRGB;
						*dst++ = (a << 24) | (r << 16) | (g << 8) | b;
					}
				}
				mImages.push_back(img);
				img = NULL;
				continue;
			}

			/*
			if ((*it).depth() > 8) {
				esyslog("ERROR: text2skin: More than 8bpp images are not supported");
				return false;
			}
			*/
			bmp = new cBitmap(w, h, std::min(size_t((*it).depth()), size_t(8)));
			//Dprintf("this image has %d colors\n", (*it).totalColors());

			for (int iy = 0; iy < h; ++iy) {
				for (int ix = 0; ix < w; ++ix) {
					tColor col = (~int(pix->opacity * 255 / MaxRGB) << 24) | (int(pix->red * 255 / MaxRGB) << 16) | (int(pix->green * 255 / MaxRGB) << 8) | int(pix->blue * 255 / MaxRGB);
					bmp->DrawPixel(ix, iy, col);
					++pix;
				}
			}
			mBitmaps.push_back(bmp);
			bmp = NULL;
		}
	} catch (Exception &e) {
		if (!Quiet)
			esyslog("ERROR: text2skin: Couldn't load %s: %s", Filename, e.what());
		delete bmp;
		delete img;
		return false;
	} catch (...) {
		if (!Quiet)
			esyslog("ERROR: text2skin: Couldn't load %s: Unknown exception caught", Filename);
		delete bmp;
		delete img;
		return false;
	}
	return true;

#else /* Not built with external image library */
	if (!Quiet)
		esyslog("ERROR: text2skin: unknown file format for %s", Filename);
	return false;

#endif
}
