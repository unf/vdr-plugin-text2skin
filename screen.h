//								-*- c++ -*-

#ifndef VDR_TEXT2SKIN_SCREEN_H
#define VDR_TEXT2SKIN_SCREEN_H

#include "common.h"
#include "setup.h"
#include <vdr/osd.h>

// Drawing surface of a true colour display. All objects are composed here
// with alpha blending, Flush() hands only the changed rectangle to the OSD.
class cText2SkinCanvas: public cPixmapMemory {
public:
	cText2SkinCanvas(const cSize &Size): cPixmapMemory(0, cRect(cPoint(0, 0), Size)) {}
	tColor *Pixels(void) { return (tColor *)Data(); }
	void MarkDirty(const cRect &Rect) { MarkDrawPortDirty(Rect); }
	void SetClean(void) { cPixmap::SetClean(); }
};

class cText2SkinScreen {
	/* Skin Editor */
	friend class VSkinnerScreen;

private:
	cOsd    *mOsd;
	cBitmap *mScreen;
	cBitmap *mRegions[MAXOSDAREAS];
	bool     mTouched[MAXOSDAREAS];
	bool     mOffScreen;
	int      mNumRegions;

	// palette mode: remember the windows a drawing operation reaches, Flush()
	// copies only those (cBitmap's dirty area misses palette changes)
	void Touch(int x1, int y1, int x2, int y2);

	// true colour mode: one OSD area around all windows, drawn via mCanvas
	cText2SkinCanvas *mCanvas;
	cPoint            mOrigin;

	// true colour mode: copy of what the OSD shows, so Flush() hands over only
	// pixels that really changed (a redraw marks everything it draws as dirty,
	// even if most of it looks the same as before)
	std::vector<tColor> mShown;
	bool                mShownValid;

	void BlendPixels(int x, int y, int Width, int Height, const tColor *Colors,
	                 const cBitmap *Bitmap, const tColor *ColorMask);

protected:
	static void DrawBitmapOverlay(cBitmap &Dest, int x, int y, cBitmap &Bitmap,
	                              const tColor *ColorMask = NULL);

public:
	cText2SkinScreen(bool OffScreen = false);
	~cText2SkinScreen();

	eOsdError SetAreas(const tArea *Areas, int NumAreas);

	void Clear(void);
	void DrawBitmap(int x, int y, const cBitmap &Bitmap, const tColor *ColorMask = NULL);
	void DrawImage(int x, int y, const cImage &Image, const tColor *ColorMask = NULL);
	void DrawRectangle(int x1, int y1, int x2, int y2, tColor Color);
	void DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font,
	              int Width = 0, int Height = 0, int Alignment = taDefault);
	void DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants = 0);
	void DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type);

	void Flush(void);

	bool IsOpen(void) const { return mOsd != NULL; }
	bool IsTrueColor(void) const { return mCanvas != NULL; }
};

#endif // VDR_TEXT2SKIN_SCREEN_H
