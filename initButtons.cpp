#include "main.h"

#define IMAGECLASS Img2
#define IMAGEFILE <Controls4U/Controls4U.iml>
#include <Draw/iml.h>

void Main::InitButtons() {
	left2.butTopPending.SetImage(Img2::TopArrow()).SetLabel("");
	left2.butUpPending.SetImage(Img2::UpArrow()).SetLabel("");
	left2.butDownPending.SetImage(Img2::DownArrow()).SetLabel("");
	left2.butBottomPending.SetImage(Img2::BottomArrow()).SetLabel("");
}