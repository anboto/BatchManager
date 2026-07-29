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

uint64 ScanBytes(const String& s) {
    double value;
    String unit;

    CParser p(s);
    value = p.ReadDouble();
    unit = Trim(p.GetPtr());

    if(unit.Find('b') >= 0)
        throw Exc("Invalid unit '" + unit + "'. Lowercase 'b' denotes bits, not bytes.");

    int power;

    if(unit == "B")
        power = 0;
    else if(unit == "kB")
        power = 1;
    else if(unit == "MB")
        power = 2;
    else if(unit == "GB")
        power = 3;
    else if(unit == "TB")
        power = 4;
    else
        throw Exc("Unknown size unit: " + unit);

    while(power-- > 0)
        value *= 1024.0;

    return (uint64)(value + 0.5);   // round to nearest byte
}