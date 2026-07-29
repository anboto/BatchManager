#include "main.h"

#define IMAGECLASS Img
#define IMAGEFILE <BatchManager/main.iml>
#include <Draw/iml.h>

 // papers_stack icon made by Freepik from www.flaticon.com 
 
GUI_APP_MAIN {
	StdLogSetup(LOG_FILE | LOG_TIMESTAMP | LOG_APPEND);
	SetLanguage(SetLNGCharset(GetSystemLNG(), CHARSET_UTF8));	
	
	Ctrl::SetAppName(t_("Batch process manager"));
	
	Main main;
	
	HANDLE mutex = ::CreateMutex(0, true, "__BatchManager__");
	if (!mutex) {
		PromptOK(t_("It is not possible to know if program is already running"));
		return;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (!PromptOKCancel(t_("Program is already running.") + String("&") + t_("Do you want to open a new one?"))) {
			CloseHandle(mutex);
			return;
		}
	}	
	
	String errorStr;
	try {
		main.Load();
		
		if (!main.Init()) {
			main.Close();
			return; 
		}
		main.OpenMain();
		Ctrl::EventLoop();
		
		main.Save();
	} catch (Exc e) {
		errorStr = e;
	} catch(const char *cad) {
		errorStr = cad;
	} catch(const std::string &e) {
		errorStr = e.c_str();	
	} catch (const std::exception &e) {
		errorStr = e.what();
	} catch(...) {
		errorStr = t_("Unknown error");
	}
	
	if (!errorStr.IsEmpty())
		Exclamation(t_("Internal error:") + String("&") + DeQtf(errorStr) + String("&") + t_("Program ended"));	
	
	main.DoClose(false);
}

Main::Main() {
	Title(t_("Batch Manager"));
	Sizeable().Zoomable();
	Icon(Img::papers_stack_16(), Img::papers_stack_128());

	CtrlLayout(output);
	CtrlLayout(main);
	
	splitter.Horz(main.SizePos(), output.SizePos());
	Add(splitter.SizePos());
	splitter.SetPositions(100, 2000, 5000).SetInitialPositionId(2);
	
	output.batchesArea.SetFrame(ThinInsetFrame());
	
	main.array.AddColumn(t_("Id"), 0);			idId		= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("Status"), 5);		idStatus	= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("File"), 8);		idFile		= main.array.GetColumnCount()-1;			
	main.array.AddColumn(t_("Args"), 2);		idArgs		= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("Folder"), 5);		idFolder	= main.array.GetColumnCount()-1;			
	main.array.AddColumn(t_("User"), 2);		idUser		= main.array.GetColumnCount()-1;			
	main.array.AddColumn(t_("Computer"), 2);	idComputer	= main.array.GetColumnCount()-1;			
	main.array.AddColumn(t_("Start"), 5);		idStart		= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("End"), 5);			idEnd		= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("MaxTime"), 5);		idMaxTime	= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("Time"), 5);		idTime		= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("Mem. Used/RAM"), 10);  idMemActual	= main.array.GetColumnCount()-1;		
	main.array.AddColumn(t_("Mem. Max."), 10);	idMemMax	= main.array.GetColumnCount()-1;		
	
	main.array.NoMovingHeader().AutoHideSb();
	main.array.SetLineCy(int(1.4*StdFont().GetCy()));
	main.array.MultiSelect();
	
	main.array.WhenDrag = THISBACK(OnDrag);
	main.array.WhenDropInsert = THISBACK(OnDropInsert);
	
	main.array.WhenBar = [&](Bar &menu) {ArrayCtrlWhenBar(menu, main.array);};	
	main.array.WhenSel = THISBACK(OnSel);
	
	main.butUp.WhenAction = THISBACK2(OnMove, -1, -1);
	main.butDown.WhenAction = THISBACK2(OnMove, 1, -1);
	main.butTop.WhenAction = THISBACK2(OnMove, -10000, -1);
	main.butBottom.WhenAction = THISBACK2(OnMove, 10000, -1);	
	
	main.butReStart.WhenAction = THISBACK(OnReStart);
	main.butEnd.WhenAction = THISBACK(OnEnd);
	main.butDelete.WhenAction = THISBACK(OnDelete);
	main.butClearEnded.WhenAction = THISBACK(ClearRealized);
	main.butFolder.WhenAction = THISBACK(OnFolder);
	
	InitButtons();
	
	main.maxCPU.WhenAction = THISBACK(OnCPUSpin);
	
	main.butPauseAll.WhenAction = [&]() {
		main.sliderProcess <<= 0;	
		main.butPauseAll.Disable();
		main.sliderProcess.WhenAction();
	};
	
	main.maxRAM.WhenAction = THISBACK(OnRAMSpin);
	
	UpdateLabs();
}

Main::~Main() {
	DoClose(false);
}

void Main::Jsonize(JsonIO& json) {
	Array<Vector<Value>> doneData;
	double version = Null;
		
	if (json.IsStoring())
		for (int i = 0; i < main.array.GetCount(); ++i)
			doneData << main.array.GetLine(i);

	json
		("version",		 version)
		("maxCPU", 		 main.maxCPU)
		("maxRAM", 		 main.maxRAM)
		("newTime", 	 main.maxTime)
		("done", 		 doneData)
		("slidermemory", main.sliderMemory)
		("sliderprocess",main.sliderProcess)
		("batch",		 batch)
	;
	if (json.IsLoading()) {
		if (IsNull(version)) {
			doneData.Clear();
			Clear();
		}
		version = 1.1;
		
		procid = 0;
		for (int row = 0; row < doneData.GetCount(); ++row)
			main.array.Set(row, doneData[row]);
		for (int row = 0; row < batch.size(); ++row) {
			output.batchesArea.Add(batch.GetIdx(row).SetParent(batch).SizePos());
			batch.GetIdx(row).Load();
		}
		procid = batch.MaxId()+1;
		UpdateLabs();
	}
}

void Main::Clear() {
	main.maxCPU <<= Null;
	main.maxRAM <<= Null;
	main.maxTime <<= Null;
	main.sliderMemory <<= Null;
	main.sliderProcess <<= Null;
	batch.Clear();
}

void Main::Load() {
	String dataDirectory = AppendFileName(GetAppDataFolder(), "BatchManager");
	RealizeDirectory(dataDirectory);
	configFile = AppendFileName(dataDirectory, "BatchManager.json");
	LoadFromJsonFile(*this, configFile);
}

void Main::Save() {
	StoreAsJsonFile(*this, configFile, true);
}

bool Main::Init() {
	String manufacturer, productName, version, mbSerial;
	int numberOfProcessors; 
	GetSystemInfo(manufacturer, productName, version, numberOfProcessors, mbSerial);	
	   
	if (IsNull(~main.maxCPU)) 
		main.maxCPU <<= numberOfProcessors;
	main.maxCPU.MinMax(1, numberOfProcessors);
	main.procTotal <<= numberOfProcessors;
	
	if (IsNull(~main.sliderProcess))
		main.sliderProcess <<= 0;
	main.sliderProcess.MinMax(0, ~main.maxCPU);
	main.sliderProcess.Step(1, true);
	main.sliderProcess.SetMajorTicks().SetMajorTicksSize(4).SetThickness(2);
	main.sliderProcess.WhenAction = [&] {main.maxProcessVal <<= ~main.sliderProcess;};
	main.sliderProcess.WhenAction();

	uint64 total, available;
	GetSystemMemoryStatus(total, available);
	const double btoGb = 1024*1024*1024;
	double dtotal = total/btoGb;	
	if (IsNull(~main.maxRAM)) 
		main.maxRAM <<= int(dtotal);
	main.maxRAM.MinMax(1, dtotal);
	
	if (IsNull(~main.sliderMemory))
		main.sliderMemory <<= 0;
	main.sliderMemory.MinMax(0, ~main.maxRAM);
	main.sliderMemory.Step(1, true);
	main.sliderMemory.SetMajorTicks().SetMajorTicksSize(4).SetThickness(2);
	main.sliderMemory.WhenAction = [&] {main.maxMemoryVal <<= ~main.sliderMemory;};
	main.sliderMemory.WhenAction();
	
	if (IsNull(~main.maxTime))
		main.maxTime <<= "240:00:00";
	main.butSetMaxTime.WhenAction = [&] {OnMaxTime();};
	
	OnCPUSpin();
	
	status << t_("Pending") << t_("Paused") << t_("Running") << t_("Ended");
	
	int pending, paused, running, ended;
	TaskCount(false, pending, paused, running, ended);
	if (pending == 0)
		main.butStart.Hide();
	
	StartCallBack();
	
	main.butStart.WhenAction = [=] {main.butStart.Hide();};
	main.labelDrop.Show(main.array.GetCount() == 0);
	main.rectangleDrop.Show(main.array.GetCount() == 0);
		
	return true;
}

void Main::StartCallBack() {
	lastScatter = lastSeries = GetSysTime();
	timeCallback.Set(-500, THISBACK(TimerFun));
}

void Main::StopCallBack() {
	timeCallback.Kill();
}

void Main::TaskCount(bool onlySelected, int &pending, int &paused, int &running, int &ended) {
	pending = 0, paused = 0, running = 0, ended = 0;
	for (int row = 0; row < main.array.GetCount(); ++row) {
		if (!onlySelected || main.array.IsSelected(row)) {
			String status = main.array.Get(row, idStatus);
			if (status == t_("Pending"))
				pending++;
			else if (status == t_("Paused"))
				paused++;
			else if (status == t_("Running"))
				running++;
			else if (status == t_("Ended"))
				ended++;
		}
	}
}
	
void Main::UpdateLabs() {
	main.labelDrop.Show(main.array.GetCount() == 0);
	main.rectangleDrop.Show(main.array.GetCount() == 0);
	
	int pending, paused, running, ended;
	TaskCount(false, pending, paused, running, ended);
	main.labTitle.SetText(F(t_(" Tasks status: Pending %d, Paused %d, Running %d, Ended %d "), pending, paused, running, ended));
	
	if (main.opSort)
		OnSort();
}

void Main::OnCPUSpin() {
	int maxCPU = Nvl(int(~main.maxCPU), 1); 
	main.sliderProcess.MinMax(0, maxCPU);
	main.sliderProcess.WhenAction();
}

void Main::OnRAMSpin() {
	int maxRAM = Nvl(int(~main.maxRAM), 1);
	main.sliderMemory.MinMax(0, maxRAM);
	main.sliderMemory.WhenAction(); 
}

void Main::OnFolder() {
	int row = main.array.GetCursor();
	if (row < 0) 
		return;
	
	LaunchFile(String(main.array.Get(row, idFolder)));
}

void Main::OnSort() {
    int n = main.array.GetCount();
    Vector<int> order;
    order.SetCount(n);
    for(int i = 0; i < n; i++)
        order[i] = i;

    StableSort(order, [&](int a, int b) {
        int ra = status.Find(main.array.Get(a, idStatus).ToString());
        int rb = status.Find(main.array.Get(b, idStatus).ToString());
        return ra < rb;
    });

	bool rearrange = false;
	for(int i = 0; i < n; i++) {
		if (order[i] != i) {
			rearrange = true;
			break;
		}
	}
	if (rearrange) {
		Index<int> selected = GetSelectedIds();
    	main.array.ReArrange(order);
    	SetSelectedIds(selected);
	}
}

void Main::OnDrag() {
	if (main.array.DoDragAndDrop(InternalClip(main.array, "array")) == DND_MOVE) {
		main.array.RemoveSelection();
		UpdateLabs();
	}
}

void Main::OnDropInsert(int line, PasteClip& d) {
	if (AcceptInternal<ArrayCtrl>(d, "array")) {
		main.array.InsertDrop(line, d);
		main.array.SetFocus();
		UpdateLabs();
	}
}

void Main::DragAndDrop(Point p, PasteClip& d) {
	if (IsDragAndDropSource())
		return;
	if (AcceptFiles(d)) {
		Vector<String> files = GetFiles(d);
		for (String file : files)
			DoDrop(file);
		Refresh();
		UpdateLabs();
	}
}

bool Main::Key(dword key, int count) {
	if(key == K_CTRL_V) {
		Vector<String> files = GetFiles(Ctrl::Clipboard());
		for (int i = 0; i < files.GetCount(); ++i)
			DoDrop(files[i]);
		Refresh();
		return true;
	}
	if (key & K_DELTA)	// Sometimes it happens
		return true;
	
	int id = batch.GetShown();
	if (id >= 0) {
		String str = WString((wchar *)&key, 1).ToString();
		batch.GetIdx(id).process.Write(str);
	}
	return true;
}

void Main::DoDrop(String name) {
	String args;
	String folder = GetFileFolder(name);
	String file = GetFileTitle(name);
	String ext = GetFileExt(name).Mid(1);
	
	if (GetFileExt(name) == ".bat") {
		String str = LoadFile(name);
		if (str.IsEmpty()) 
			if (!PromptOKCancel(F(t_("File %s is empty.&") + String(t_("Do you want to continue?")), DeQtf(name))))
				return;
		if (ToLower(str).Find("pause") >= 0) {
			if (!PromptOKCancel(F(t_("File %s has a PAUSE command.&") + String(t_("Do you want to continue?")), DeQtf(name))))
				return;
		}
	} else if (GetFileExt(name) == ".bat")
		;
	else {
		struct HandledTypes {
			String pattern, command, args;
		};
		Array<HandledTypes> data = {{"*analysis.dat", "bemrosetta_cl", "-aqwa -r \"#PATH#\""},
									{"*.dat",         "bemrosetta_cl", "-orca -numtries 10 -timelog 10 -rf \"#PATH#\" \"#FOLDER#\\#FILE#.sim\""}};		
		
		for (const HandledTypes &h : data) {
			if (PatternMatch(h.pattern, name)) {
				args = h.args;				
				args.Replace("#PATH#", name);
				args.Replace("#FOLDER#", folder);
				args.Replace("#FILE#", file);
				args.Replace("#EXT#", ext);
				name = h.command;
			}
		}
		if (args.IsEmpty())
			return;
	}

	main.array.Add(procid, t_("Pending"), GetFileName(name), args, folder, GetUserName(), GetComputerName(), "", "",
				SecondsToString(StringToSeconds(String(~main.maxTime)), 0, false, false, true, false, true));
	UpdateLabs();
	
	output.batchesArea.Add(batch.Add(procid).SizePos());
	
	main.array.SetCursor(main.array.GetCount() - 1);
	main.array.SetFocus();
	
	procid++;
}

Index<int> Main::GetSelectedRows() {
	Index<int> ret;
	for (int row = 0; row < main.array.GetCount(); row++)
		if (main.array.IsSelected(row))
			ret << row;
	return ret;
}

Index<int> Main::GetSelectedIds() {
	Index<int> ret;
	for (int row = 0; row < main.array.GetCount(); row++)
		if (main.array.IsSelected(row))
			ret << main.array.Get(row, idId);
	return ret;
}

void Main::SetSelectedIds(const Index<int> &ids) {
	for (int row = 0; row < main.array.GetCount(); row++)
		if (ids.Find(main.array.Get(row, idId)) >= 0)
			main.array.Select(row);
}

void Main::ClearRealized() {
	for (int i = main.array.GetCount(); i >= 0; --i) {
		if (String(main.array.Get(i, idStatus)) == t_("Ended")) {
			batch.Remove(main.array.Get(i, idId));
			main.array.Remove(i);
		}
	}
	UpdateLabs();
}

void Main::OnReStart() {
	Index<int> sels = GetSelectedRows();
	if (sels.IsEmpty())
		return;

	bool domessage = false;
	for (int i = 0; i < sels.size(); ++i) {
		String status = String(main.array.Get(i, idStatus));
		if (status == t_("Ended"))
			main.array.Set(sels[i], idStatus, t_("Pending"));
		else
			domessage = true;
	}
	if (domessage)
		Exclamation(t_("Only ended processes have been re-started"));
	
	UpdateLabs();
}


void Main::OnEnd() {
	Index<int> sels = GetSelectedRows();
	if (sels.IsEmpty())
		return;
	
	bool domessage = false;
	for (int i = 0; i < sels.size(); ++i) {
		String status = String(main.array.Get(i, idStatus));
		if (status == t_("Running") || status == t_("Paused"))
			main.array.Set(sels[i], idStatus, t_("Ended"));
		else
			domessage = true;
	}
	if (domessage)
		Exclamation(t_("Only Running processes have been ended"));
	
	UpdateLabs();	
}
	
void Main::OnDelete() {
	Index<int> sels = GetSelectedRows();
	if (sels.IsEmpty())
		return;
	
	bool domessage = false;
	for (int i = sels.size()-1; i >= 0; --i) {
		String status = String(main.array.Get(i, idStatus));
		if (status == t_("Ended") || status == t_("Pending")) {
			batch.Remove(main.array.Get(sels[i], idId));
			main.array.Remove(sels[i]);
		} else
			domessage = true;
	}
	if (domessage)
		Exclamation(t_("Only Pending and Ended processes have been removed"));
	
	UpdateLabs();
}

void Main::OnMaxTime() {
	Index<int> sels = GetSelectedRows();
	if (sels.IsEmpty()) 
		return;
	
	String strTime = ~main.maxTime;
	double maxTime = StringToSeconds(strTime);
	if (IsNull(maxTime) || maxTime <= 10)
		return;

	for (int i = 0; i < sels.size(); ++i) {
		int row = sels[i];
		int id = main.array.Get(row, idId); 
		
		String status = main.array.Get(row, idStatus);
		
		main.array.Set(row, idMaxTime, SecondsToString(maxTime, 0, false, false, true, false, true));	
		if (status == t_("Paused") || (status == t_("Running")))
			batch.GetId(id).process.SetMaxRunTime(maxTime);
	}
}		   

void Main::OnMove(int delta, int processingrow) {
	Vector<int> idss;		// Unique identifiers
	if (processingrow >= 0) {
		int id = main.array.Get(processingrow, idId); 	
		idss << id;
	} else {
		Index<int> sels = GetSelectedRows();
		if (sels.IsEmpty()) 
			return;
	
		for (int i = 0; i < sels.size(); ++i) {
			int id = main.array.Get(sels[i], idId); 	
			idss << id;
		}
	}
	if (delta == -10000)
		Reverse(idss);
	
	for (int i = 0; i < idss.size(); ++i) {
		for (int row = 0; row < main.array.GetCount(); ++row) {
			int id = main.array.Get(row, idId); 
			if (id == idss[i]) {
				int ddelta = delta;
				if (row + delta < 0)
					ddelta = -row;
				else if (row + delta >= main.array.GetCount())
					ddelta = main.array.GetCount() - row - 1;
				
				if (ddelta != 0) {
					main.array.SetCursor(row);
					int dd = abs(ddelta);
					ddelta = ddelta/dd;
					for (; dd > 0; --dd)
						main.array.Move(ddelta);
				}
				break;
			}
		}
	}
	for (int row = 0; row < main.array.GetCount(); ++row) {
		int id = main.array.Get(row, idId); 
		if (Find(idss, id) >= 0)
			main.array.Select(row);
	}
}

void Main::Close() { 
	DoClose(true);	
}

void Main::DoClose(bool prompt) {
	if (batch.IsRunning() && prompt) {	
		if(!PromptYesNo(t_("There are processes opened.&Do you want to end BatchManager?")))
			return;
	
		if(PromptYesNo(t_("Do you want to cancel all processes?"))) {
			batch.Stop();
			for (int row = 0; row < main.array.GetCount(); ++row)
				main.array.Set(row, idStatus, t_("Ended"));
		}
	}
	StopCallBack();
	RejectBreak(IDOK);		// Empty EditStrings does not disturb
	TopWindow::Close();	
}

void Main::OnSel() {
	NON_REENTRANT_V;

	int pending, paused, running, ended;
	
	TaskCount(true, pending, paused, running, ended);
	int numsel = pending + paused + running + ended;
	
	main.butFolder.Enable(numsel == 1);
	main.butEnd.Enable(paused + running > 0);
	main.butReStart.Enable(ended > 0);
	main.butDelete.Enable(pending + ended > 0);
	main.butTop.Enable(numsel > 0);
	main.butUp.Enable(numsel > 0);
	main.butDown.Enable(numsel > 0);
	main.butBottom.Enable(numsel > 0);
	main.butSetMaxTime.Enable(numsel > 0);
	
	TaskCount(false, pending, paused, running, ended);
	main.butPauseAll.Enable(int(~main.sliderProcess) > 0);
	main.butClearEnded.Enable(ended > 0);
	
	int row = main.array.GetCursor();
	if (row < 0) 
		return;
	
	batch.Show(main.array.Get(row, idId));
}

void Main::TimerFun() {
	NON_REENTRANT_V;

	if (main.butStart.IsShown())
		return;	
	
	Time now = GetSysTime();
		
	uint64 total, available;
	GetSystemMemoryStatus(total, available);
	const double BtoGB = 1024*1024*1024;
	double dtotal = total/BtoGB;
	double dused = (total-available)/BtoGB;
	main.memTotal = F("%.1f", dtotal);
	main.memUsed  = F("%.1f", dused);
	double ratioMem = dused/dtotal;
	Color col;
	if (ratioMem < 0.75)
		col = LtGreen();
	else if (ratioMem < 0.9)
		col = Yellow();
	else
		col = LtRed();
	main.memUsed.SetBackground(col);
	
	int cpuAvailable = ~main.sliderProcess;
	double memAvailable = double(~main.sliderMemory)*BtoGB;
	
	uint64 memTotal = 0, memMax = 0;
	
	bool updateScatter = false, updateSeries = false;
	if (now - lastScatter > 5) {
		updateScatter = true;
		lastScatter = now;
	}
	if (now - lastSeries > 1) {
		updateSeries = true;
		lastSeries = now;
	}
			
	for (int row = 0; row < main.array.GetCount(); ++row) {
		String status = main.array.Get(row, idStatus);
		Task &task = batch.GetId(main.array.Get(row, idId));
		
		bool endTask = false;
		if (status == t_("Paused") || status == t_("Running")) {		// array data update
			task.Perform(updateScatter, updateSeries);
			
			main.array.Set(row, idTime, 	 SecondsToString(task.process.Seconds(), 0, false, false, true, false, true));

			String smemTotal = task.memTotal == 0 ? "" : FormatBytes(task.memTotal);
			String smemRam   = task.memRam   == 0 ? "" : FormatBytes(task.memRam);
			if (task.memTotal == 0 && task.memRam == 0 )
				main.array.Set(row, idMemActual, "");
			else
				main.array.Set(row, idMemActual, smemTotal + "/" + smemRam);
			main.array.Set(row, idMemMax, task.memMax   == 0 ? "" : FormatBytes(task.memMax));
		}
		
		auto StartIfPossible = [&](bool firstTime) {
			if (cpuAvailable > 0 && memAvailable > task.memMax && (IsNull(task.lastPausedStarted) || now - task.lastPausedStarted > 10)) {
				if (firstTime) {
					String folder = main.array.Get(row, idFolder);
					String file = main.array.Get(row, idFile);
					String args = main.array.Get(row, idArgs);
					double maxTime = StringToSeconds(String(main.array.Get(row, idMaxTime)));		
					task.Start(folder, file, args, StringToSeconds(String(main.array.Get(row, idMaxTime))));
				} else
					task.process.Pause();
				
				main.array.Set(row, idStatus, t_("Running"));
				if (!firstTime)
					task.output.Print(F("\n") + t_("Program is RUNNING"), Green());
				else
					main.array.Set(row, idStart, now);
				
				cpuAvailable--;
				memAvailable -= task.memMax;
				task.lastPausedStarted = now;
			}
		};
			
		if (status == t_("Pending"))
			StartIfPossible(true);
		else if (status == t_("Paused")) {
			if (!task.process.IsPaused()) {
				task.process.Pause();
				task.output.Print(F("\n") + t_("Program is PAUSED"), Green());	
			} else
				StartIfPossible(false);
		} else if (status == t_("Running")) {
			if (task.process.GetStatus() != LocalProcessX::RUNNING) {
				endTask = true;
				main.array.Set(row, idStatus, t_("Ended"));
			} else if (cpuAvailable == 0 || memAvailable < task.memMax) { 
				if (!task.process.IsPaused() && now - task.lastPausedStarted > 10) {
					task.process.Pause();
					task.Perform(false, false);					// To print pending text
					task.output.Print(F("\n") + t_("Program is PAUSED"), Green());	
					main.array.Set(row, idStatus, t_("Paused"));
					task.lastPausedStarted = now;
				}
			} else if (task.process.IsPaused()) {
				task.process.Pause();
				cpuAvailable--;
				memAvailable -= task.memMax;
			} else {
				cpuAvailable--;
				memAvailable -= task.memMax;
			}
		} else if (status == t_("Ended")) {
			if (task.process.GetStatus() == LocalProcessX::RUNNING) {
				task.process.Stop();
				endTask = true;
			}
		}
		if (endTask) {
			String msg;
			int pstatus = task.process.GetStatus();
			switch(pstatus) {
			case LocalProcessX::STOP: 	 	 	msg = F(t_("Program ended. Exit code %d"), task.process.GetExitCode());	break;
			case LocalProcessX::STOP_USER: 	 	msg = t_("Program stopped by user");			break;
			case LocalProcessX::STOP_TIMEOUT:	msg = t_("Execution time exceeded (timeout)");	break;
			case LocalProcessX::STOP_NORESPONSE:msg = t_("Application does not respond");		break;	
			}
			task.output.Print("\n" + msg + "\n", Green());	
			main.array.Set(row, idEnd, now);
			task.lastPausedStarted = Null;
		}
		if (status == t_("Running")) {
			memTotal += task.memTotal;
			memMax   += task.memMax;
		}
	}
	UpdateLabs();
	
	OnSel();
	
	main.memUsedUser = F("%.1f", memTotal/BtoGB);
	main.memMaxUser  = F("%.1f", memMax/BtoGB);
}	


