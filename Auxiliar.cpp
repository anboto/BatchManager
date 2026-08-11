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