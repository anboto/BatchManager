#include "main.h"

Task::Task() {
	CtrlLayout(*this);
	
	output.SetFont(Font(Font::MONOSPACE, GetStdFont().GetHeight()));
	output.SetReadOnly();
	
	scatter.SetFastViewX(true).SetSequentialXAll(true);
	scatter.ShowInfo().ShowContextMenu().ShowPropertiesDlg().ShowProcessDlg();	
	scatter.SetLabelX("Time [s]").SetLabelY("Process Memory [MB]").SetLabelY2("Paging per second");
	scatter.SetLegendFillColor(Null).SetLegendAnchor(ScatterDraw::LEFT_TOP);
	
	opAutoUpdate = true;
	opAutoUpdate.Disable();
	opShowRAM.Disable();
	opShowPaging.Disable();
	
	opAutoUpdate.WhenAction = THISBACK(UpdateScatters);
	opShowRAM.WhenAction    = THISBACK(UpdateScatters);
	opShowPaging.WhenAction = THISBACK(UpdateScatters);
	
	opAutoUpdate.Enable();
	opShowRAM.Enable();
	opShowPaging.Enable();
}

void Task::UpdateScatters() {
	if (!parent)
		return;
	
	for (int i = 0; i < parent->size(); ++i) {
		Task &b = parent->GetIdx(i);
		
		b.opAutoUpdate = ~opAutoUpdate;
		if (opAutoUpdate) 
			b.scatter.ZoomToFit(true, true, 0, 0.1);
			
		b.opShowRAM = ~opShowRAM;
		if (b.scatter.ScatterDraw::IsValid(1))
			b.scatter.ScatterDraw::Show(1, ~opShowRAM);
		
		b.opShowPaging = ~opShowPaging;
		if (b.scatter.ScatterDraw::IsValid(2))
			b.scatter.ScatterDraw::Show(2, ~opShowPaging);
	}
}

void Task::Load() {
	vtime.Clear();
	vtotal.Clear();
	vram.Clear();
	vpaging_s.Clear();
	
	vtime << 0;
	vtotal << 0;
	vram << 0;
	vpaging_s << 0;
	
	scatter.RemoveAllSeries();
	scatter.AddSeries(vtime, vtotal).Legend("Total Allocated").Units("MB", "s").NoMark().Stroke(1.5);
	scatter.AddSeries(vtime, vram).Legend("Physical RAM").Units("MB", "s").NoMark().Stroke(1.5);
	scatter.AddSeries(vtime, vpaging_s).Legend("Paging").Units("Paging/s", "s").NoMark().SetRightY().Stroke(1.5);
	
	opShowRAM.WhenAction();
	opShowPaging.WhenAction();
	
	scatter.ZoomToFit(true, true, 0, 0.1);	
}

void Task::Start(String folder, String file, String args, double maxTime) {
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
	
	process.WhenTimer = THISBACK(OnProcess);
	process.Start(command, nullptr, nullptr, -1, maxTime, maxTime);
	
	Load();
}

void Task::Perform(bool updateScatter, bool updateSeries) {
	process.Perform();
	
	if (!process.GetMemory(memTotal, memRam, paging_s))
		memTotal = memRam = paging_s = 0;
	else
		memMax = max(memMax, memTotal);
	
	if (updateSeries || updateScatter) {
		vtime  << int(GetSysTime() - process.GetStart());
		vtotal << int(memTotal/1024/1024);
		vram   << int(memRam/1024/1024);
		vpaging_s << paging_s;
	}
	if (updateScatter && opAutoUpdate)
		scatter.ZoomToFit(true, true, 0, 0.1);		
}

void Task::OnStop() {
	if(!PromptYesNo(t_("Do you want to stop the process?")))
		return;
	//if (process.IsPaused()) 
		//Pause();
	process.Stop();
	
	opAutoUpdate.Disable();
	opShowRAM.Disable();
	opShowPaging.Disable();
}

/*
void Task::Stop() {
	process.Stop();
}

void Task::Pause() {
	process.Pause();
	String msg;
	if (process.IsPaused())
		msg = t_("Process is PAUSED");
	else
		msg = t_("Process is RUNNING");

	lastPausedStarted = GetSysTime();
	output.Print("\n" + msg + "\n", Green());	
}*/

void Task::Jsonize(JsonIO& json) {
	String console;
	int64 _memMax, _memTotal, _memRam;
	
	if (json.IsStoring()) {
		console = output.Get();
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
		output.Set(console);
		memMax = _memMax;
		memTotal = _memTotal;
		memRam = _memRam;
	}
}

bool Task::OnProcess(double, const String &out, bool isEnd, bool &resetTimeout) {
	output.Print(out, Blue());
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
	return tasks[index].output.Get();
}

void Batch::SetConsole(int id, String str) {
	int index = GetIndex(id);
	if (index < 0)
		return;
	tasks[index].output.Set(str);
}

void Batch::Jsonize(JsonIO& json) {
	json
		("tasks",   tasks)
	;
}
