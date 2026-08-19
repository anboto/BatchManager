// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 - 2026, the BatchManager author
#include "main.h"

#define IMAGECLASS Img2
#define IMAGEFILE <Controls4U/Controls4U.iml>
#include <Draw/iml.h>

void Main::InitButtons() {
	main.butTop.SetImage(Img2::TopArrow()).SetLabel("");
	main.butUp.SetImage(Img2::UpArrow()).SetLabel("");
	main.butDown.SetImage(Img2::DownArrow()).SetLabel("");
	main.butBottom.SetImage(Img2::BottomArrow()).SetLabel("");
}

MySpinButtons::MySpinButtons() {
    Add(up);
    Add(down);
    
    up << [=] {
        if (IsNull(max) || int(GetData()) < max) {
    		SetData(int(GetData()) + 1);
    		if (WhenAction)
    			WhenAction();
        }
    };
    down << [=] {
        if (IsNull(max) || int(GetData()) > min) {
    		SetData(int(GetData()) - 1);
    		if (WhenAction)
    			WhenAction();
        }
    };
}
   
void MySpinButtons::Layout() {
    Size sz = GetSize();
    if (sz.cx == 0 || sz.cy == 0)
       return;
    
    int h = sz.cy/2;
    up.SetRect(0, 0, sz.cx, h);
    down.SetRect(0, h, sz.cx, sz.cy - h);
    
    Size bsz = up.GetSize();
    up.SetImage  (Rescale(CtrlsImg::SpU(), bsz.cx/3, bsz.cy/3));
    down.SetImage(Rescale(CtrlsImg::SpD(), bsz.cx/3, bsz.cy/3));
}

void MySpinButtons::MinMax(int min, int max) {
	this->min = min;
	this->max = max;
	if (data < min)
		data = min;
	if (data > max)
		data = max;
} 
 
String FormatBytes(uint64 bytes) {
    static const char* suffixes[] = { "B", "kB", "MB", "GB", "TB" };
    int suffixIndex = 0;
    double size = (double)bytes;

    while (size >= 1024 && suffixIndex < 4) {
        size /= 1024;
        suffixIndex++;
    }
    return F("%.1f %s", size, suffixes[suffixIndex]);
}

void AddToPATH(const Vector<String>& addFolders) {
#ifdef PLATFORM_WIN32
    const char pathsep = ';';
    const bool case_sensitive = false;
#else  // Linux and macOS
    const char pathsep = ':';
    const bool case_sensitive = true;
#endif

    String oldpath = GetEnv("PATH");
    Vector<String> existing = Split(oldpath, pathsep);

    auto AlreadyThere = [&](const String& folder) {
        for(const String& e : existing) {
            if(case_sensitive ? (e == folder) : (ToUpper(e) == ToUpper(folder)))
                return true;
        }
        return false;
    };
    String newpart;
    for(const String& f : addFolders) {
        if(f.IsEmpty() || AlreadyThere(f))
            continue;
        if(!newpart.IsEmpty())
            newpart.Cat(pathsep);
        newpart << f;
        existing << f;	// avoid duplicates within addFolders itself too 
    }
    if(newpart.IsEmpty())
        return;

    String newpath = newpart;
    if(!oldpath.IsEmpty())
        newpath << pathsep << oldpath;

    SetEnv("PATH", newpath);
}

void GreenRunningDisplay::Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const {
    bool selected = paper == SColorHighlight();
    String text = q.ToString();
    Color backColor;
    if (text == t_("Running"))
    	backColor = Color(220, 255, 220);
    else if (text == t_("Ended"))
    	backColor = Color(220, 255, 255);
    else if (text == t_("Paused"))
    	backColor = Color(255, 255, 220);
    else
        backColor = paper;
   
    Color textColor;
    if (!selected)
        textColor = Black();
    else {
        textColor = Color(80, 80, 80);
        backColor = Blend(backColor, SColorHighlight(), 128);
    };
    
    w.DrawRect(r, backColor);
    Size sz = GetTextSize(text, StdFont());
    int x = r.left + (r.Width()  - sz.cx) / 2;      // Horizontally centered
    int y = r.top  + (r.Height() - sz.cy) / 2;     	// Vertically centered
    w.DrawText(x, y, text, StdFont(), textColor);
}
