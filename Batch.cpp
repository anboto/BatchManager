// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 - 2026, the BatchManager author
#include "main.h"

Task::Task() {
	splitter.Add(console.SizePos(), 0, 0);
	splitter.Add(scatter.SizePos(), 1, 0);
	splitter.SetHeights({4, 2});
	Add(splitter.SizePos());

	console.SetFont(Font(Font::MONOSPACE, GetStdFont().GetHeight()));
	//console.SetReadOnly();
	console.OnKey =[=](dword key, int count) {
		String skey = WString((wchar *)&key, 1).ToString();	// Keys are sent to the console
		String str;
		for (int i = 0; i < count; ++i)
			str << skey;
		process.Write(str);
	};
	
	scatter.SetFastViewX(true).SetSequentialXAll(true);
	scatter.ShowInfo().ShowContextMenu().ShowPropertiesDlg().ShowProcessDlg();	
	scatter.SetLabelX(t_("Time")).SetLabelY(t_("Process Memory")).SetLabelY2(t_("RAM Paging"));
	scatter.SetLegendFillColor(Null).SetLegendAnchor(ScatterDraw::LEFT_TOP);
	scatter.SetMargin(90, 90, 20, 50);
}

void Task::LoadNew() {
	vtime.Clear();
	vtotal.Clear();
	vram.Clear();
	vpaging_s.Clear();
	
	vtime << 0;
	vtotal << 0;
	vram << 0;
	vpaging_s << 0;

	LoadLoaded();
}

void Task::LoadLoaded() {
	scatter.RemoveAllSeries();
	scatter.AddSeries(vtime, vtotal).Legend(t_("Total Allocated")).Units("MB", "s").NoMark().Stroke(1.5);
	scatter.AddSeries(vtime, vram).Legend(t_("Physical RAM")).Units("MB", "s").NoMark().Stroke(1.5);
	scatter.AddSeries(vtime, vpaging_s).Legend(t_("Paging")).Units(t_("Paging/s"), "s").NoMark().SetRightY().Stroke(1.5);
	
	UpdateScatter(true);
}

void Task::Start(String folder, String file, String args, double maxTime, const Vector<String> &path) {
	String driveLetter = "";
	if (folder[1] == ':')
		driveLetter = folder.Left(2) + " && ";
	String command;
	if (GetFileExt(file) == ".bat") 
		command = String("cmd.exe /c \"") + driveLetter + String("cd \"") + folder + "\" && \"" + file + "\"\"";
	else {
		if (FileExists(AFX(folder, file))) 
			command = AppendFileName(folder, file); 
		else
			command = file;
		command << " " << args;
	}
	
	if (!path.IsEmpty())
		AddToPATH(path);
	
	process.WhenTimer = THISBACK(OnProcess);
	process.Start(command, nullptr, nullptr, -1, maxTime, maxTime);
	
	LoadNew();
}

void Task::Perform(bool updateScatter, bool forceZoom, bool updateSeries) {
	process.Perform();
	
	if (!process.GetMemory(memTotal, memRam, paging_s))
		memTotal = memRam = paging_s = 0;
	else {
		if (memTotal > memMax) {
			memMax = memTotal;
			updateSeries = true;		// To get the events
		}
	}
	if (updateSeries) {
		vtime  << int(GetSysTime() - process.GetStart());
		vtotal << int(memTotal/1024/1024);
		vram   << int(memRam/1024/1024);
		vpaging_s << paging_s;
	}
	if (updateScatter) {
		if (forceZoom) 
			scatter.ZoomToFit(true, true, 0, 0.1);
		else
			scatter.Refresh();
	}
}

void Task::OnStop() {
	if(!PromptYesNo(t_("Do you want to stop the process?")))
		return;

	process.Stop();
}

void Task::Jsonize(JsonIO& json) {
	int64 _memMax, _memTotal, _memRam;
	
	if (json.IsStoring()) {
		_memMax = memMax;
		_memTotal = memTotal;
		_memRam = memRam;
	}
	
	json
		("id", 		  id)
		("console",   console)
		("vtime",     vtime)
		("vtotal",    vtotal)
		("vram",      vram)
		("vpaging_s", vpaging_s)
		("memMax",	  _memMax)
		("memTotal",  _memTotal)
		("memRam",	  _memRam)
		("paging_s",  paging_s)
	;

	if (json.IsLoading()) {
		memMax = _memMax;
		memTotal = _memTotal;
		memRam = _memRam;
		LoadLoaded();
	}
}

void Task::UpdateScatter(bool forceZoom) {
	if (!parent)
		return;
	
	if (!parent->output.opShowMemory) {
		splitter.SetHeights({1, 0});
		return;	
	} 
	splitter.SetHeights({4, 2});
	
	if (scatter.ScatterDraw::IsValid(1))
		scatter.ScatterDraw::Show(1, parent->output.opShowRAM);
	
	if (scatter.ScatterDraw::IsValid(2))
		scatter.ScatterDraw::Show(2, parent->output.opShowPaging);
	
	if (forceZoom) 
		scatter.ZoomToFit(true, true, 0, 0.1);
	else
		scatter.Refresh();
}

bool Task::OnProcess(double, const String &out, bool isEnd, bool &resetTimeout) {
	console.Print(out, Blue());
	return true;
}

int Batch::GetIndex(int id) {
	for (int i = 0; i < tasks.size(); ++i) {
		if (tasks[i].id == id)
			return i;
	}
	return -1;
}

String Batch::GetConsole(int id) {
	int index = GetIndex(id);
	if (index < 0)
		return String();
	return tasks[index].console.Get();
}

void Batch::SetConsole(int id, String str) {
	int index = GetIndex(id);
	if (index < 0)
		return;
	tasks[index].console.Set(str);
}

void Batch::Jsonize(JsonIO& json) {
	json
		("tasks",   tasks)
	;
}
