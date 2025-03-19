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
		if (!PromptOKCancel(t_("Program is already running.") + String("&") + t_("Do you want to continue?"))) {
			CloseHandle(mutex);
			return;
		}
		//main.alreadyRunning = true;
	}	
	
	String errorStr;
	try {
		main.Load();
		
		if (!main.Init()) {
			main.Close();
			return; 
		}
		main.Start();
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
	main.Stop();
	
	if (!errorStr.IsEmpty())
		Exclamation(t_("Internal error:") + String("&") + DeQtf(errorStr) + String("&") + t_("Program ended"));	
	
	main.DoClose(false);
}

Batch::Batch() : isStarted(false), isEnded(false) {
	CtrlLayout(*this);
	
	output.SetFont(Font(Font::MONOSPACE, GetStdFont().GetHeight()));
	output.SetReadOnly();
	
	process.WhenTimer = THISBACK(OnProcess);
	butStop.WhenAction = THISBACK(OnStop);
	butPause.WhenAction = THISBACK(OnPause);
	butPause.SetLabel(t_("Pause"));
	butCarpeta.WhenAction = THISBACK(OnFolder);
	butLater.WhenAction = THISBACK(OnLater);
}

void Batch::OnStop() {
	if(!PromptYesNo(t_("Do you want to stop the process?")))
		return;
	if (process.IsPaused()) 
		OnPause();
	Stop();
}

void Batch::Stop() {
	process.Stop();
}

void Batch::OnLater() {
	if (process.IsPaused()) 
		OnPause();
	Stop();	
	process.SetData(t_("Continue later"));
}

void Batch::OnPause() {
	process.Pause();
	butPause.SetLabel(process.IsPaused() ? t_("Continue") : t_("Pause"));
	String msg;
	if (process.IsPaused()) {
		msg = t_("Process is PAUSED");
		process.SetData(t_("Process is PAUSED"));
	} else {
		msg = t_("Process is RUNNING");
		process.SetData(t_("Process is RUNNING"));
	}
	output.Print("\n" + msg + "\n", Green());	
}

void Batch::OnFolder() {
	LaunchFile(String(~folder));
}

bool Batch::OnProcess(double, const String &out, bool isEnd, 	bool &resetTimeout) {
	output.Print(out, Blue());
	return true;
}
		
Main::Main() {
	Title(t_("Batch Manager"));
	Sizeable().Zoomable();
	Icon(Img::papers_stack_16(), Img::papers_stack_128());
	
	CtrlLayout(right);
	CtrlLayout(left1);
	CtrlLayout(left2);
	CtrlLayout(left3);
	splitter1_2.Vert(left1.SizePos(), left2.SizePos());
	splitter12_3.Vert(splitter1_2.SizePos(), left3.SizePos());
	splitter.Horz(splitter12_3.SizePos(), right.SizePos());
	Add(splitterH.SizePos());
	splitterH.Vert(splitter.SizePos(), console.SizePos());
	splitter.SetPositions(100, 2000, 5000).SetInitialPositionId(2);
	splitterH.SetPositions(8000, 9900).SetInitialPositionId(1);
	splitter1_2.SetPositions(4000).SetInitialPositionId(0);
	splitter12_3.SetPositions(5000).SetInitialPositionId(0);
	
	right.batchesArea.SetFrame(ThinInsetFrame());
	
	left1.processing.AddColumn(t_("Id"), 2);				proc.Add(idId);
	left1.processing.AddColumn(t_("File"), 8);				proc.Add(idFile);
	left1.processing.AddColumn(t_("Args"), 2);				proc.Add(idArgs);
	left1.processing.AddColumn(t_("Folder"), 5);			proc.Add(idFolder);
	left1.processing.AddColumn(t_("User"), 2);				proc.Add(idUser);
	left1.processing.AddColumn(t_("Computer"), 2);			proc.Add(idComputer);
	left1.processing.AddColumn(t_("CPU"), 2);				proc.Add(idCPU);
	left1.processing.AddColumn(t_("Begin"), 10);			proc.Add(idBegin);
	left1.processing.AddColumn(t_("Time/MaxTime"), 10);		proc.Add(idTime);
	left1.processing.AddColumn(t_("Memory Actual/Avg"), 10);proc.Add(idMem);
	left1.processing.AddColumn(t_("Status"), 5);			proc.Add(idStatus);
	left1.processing.WhenDropInsert = THISBACK(DnDInsertProc);
	left1.processing.WhenDrag = THISBACK(DragProc);
	left1.processing.SetLineCy(int(1.4*StdFont().GetCy())).MultiSelect();
	left1.processing.WhenBar = [&](Bar &menu) {ArrayCtrlWhenBar(menu, left1.processing);};	
	left1.processing.WhenSel = THISBACK1(OnSel, 0);
		
	left2.pending.AddColumn(t_("Id"), 2);					pend.Add(idId);
	left2.pending.AddColumn(t_("File"), 8);					pend.Add(idFile);
	left2.pending.AddColumn(t_("Args"), 2);					pend.Add(idArgs);
	left2.pending.AddColumn(t_("Folder"), 5);				pend.Add(idFolder);
	left2.pending.AddColumn(t_("User"), 2);					pend.Add(idUser);
	left2.pending.AddColumn(t_("Computer"), 2);				pend.Add(idComputer);
	left2.pending.AddColumn(t_("CPU"), 2);					pend.Add(idCPU);
	left2.pending.AddColumn(t_("MaxTime"), 5);				pend.Add(idTime);
	left2.pending.WhenDropInsert = THISBACK(DnDInsertPend);
	left2.pending.WhenDrag = THISBACK(DragPend);
	left2.pending.WhenDrop = THISBACK(DnD);
	left2.pending.WhenSel = THISBACK1(OnSel, 2);
	left2.pending.SetLineCy(int(1.4*StdFont().GetCy())).MultiSelect();
	left2.pending.WhenBar = [&](Bar &menu) {ArrayCtrlWhenBar(menu, left2.pending);};
	left2.butDeletePending.WhenAction = THISBACK(OnPendingsDelete);
	left2.pendCPU.WhenAction = THISBACK(OnPendingsCPU);
	left2.pendTime.WhenAction = THISBACK(OnPendingsTime);
	left2.butUpPending.WhenAction = THISBACK2(OnMove, -1, -1);
	left2.butDownPending.WhenAction = THISBACK2(OnMove, 1, -1);
	left2.butTopPending.WhenAction = THISBACK2(OnMove, -10000, -1);
	left2.butBottomPending.WhenAction = THISBACK2(OnMove, 10000, -1);
	InitButtons();
	
	left3.done.AddColumn(t_("Id"), 2).Sorting();			done.Add(idId);
	left3.done.AddColumn(t_("File"), 8).Sorting();			done.Add(idFile);
	left3.done.AddColumn(t_("Args"), 2);					done.Add(idArgs);
	left3.done.AddColumn(t_("Folder"), 5);					done.Add(idFolder);
	left3.done.AddColumn(t_("User"), 2);					done.Add(idUser);
	left3.done.AddColumn(t_("Computer"), 2);				done.Add(idComputer);
	left3.done.AddColumn(t_("Begin"), 5).Sorting();			done.Add(idBegin);
	left3.done.AddColumn(t_("End"), 5).Sorting();			done.Add(idEnd);
	left3.done.AddColumn(t_("MaxTime"), 5);					done.Add(idTime);
	left3.done.AddColumn(t_("Duration"), 5).Sorting();		done.Add(idDuration);
	left3.done.AddColumn(t_("Memory"), 10);					done.Add(idMemory);
	left3.done.WhenDrag = THISBACK(DragDone);
	left3.done.SetLineCy(int(1.4*StdFont().GetCy())).MultiSelect();
	left3.done.WhenBar = [&](Bar &menu) {ArrayCtrlWhenBar(menu, left3.done);};
	left3.done.WhenSel = THISBACK1(OnSel, 1);
	left3.done.WhenBar = [&](Bar &menu) {ArrayCtrlWhenBar(menu, left3.done);};
	
	left3.maxCPU.WhenAction = THISBACK(OnCPUSpin);
	left3.butClear.WhenAction = THISBACK(ClearRealized);
	
	left3.butPause.WhenAction = [&]() {left3.maxSimult <<= 0;	left3.maxSimult.WhenAction();};
	
	UpdateLabs();
	
	console.SetFont(Courier());
	console.SetReadOnly();
	
	//alreadyRunning = false;
}

Main::~Main() {
	DoClose(false);
}

void Main::UpdateLabs() {
	int paused = 0;
	for (int i = 0; i < left1.processing.GetCount(); ++i) {
		if (left1.processing.Get(i, proc(idStatus)) == t_("Paused"))
			paused++;
	}
	if (paused == 0)
		left1.labProcessing.SetText(Format(t_("Processing: %d"), left1.processing.GetCount()));
	else
		left1.labProcessing.SetText(Format(t_("Processing: %d (paused: %d)"), left1.processing.GetCount(), paused));
	
	left2.labPending.SetText(Format(t_("Pending: %d"), left2.pending.GetCount()));
	left3.labDone.SetText(Format(t_("Done: %d"), left3.done.GetCount()));
}

void Main::OnCPUSpin() {
	int maxCPU = Nvl(int(~left3.maxCPU), 1);
	left2.pendCPU <<= BetweenVal(int(~left2.pendCPU), 1, maxCPU);
	left2.pendCPU.MinMax(1, maxCPU);
	left3.maxSimult.WhenAction(); 
	left3.maxSimult.MinMax(0, maxCPU);
}

void Main::Jsonize(JsonIO& json) {
	Array<Vector<Value>> doneData;
	
	if (json.IsStoring()) {
		/*pendingToJsonize.Clear();
		for (int i = 0; i < left1.processing.GetCount(); ++i) {
			String file = left1.processing.Get(i, 1);
			String folder = left1.processing.Get(i, 3);
			pendingToJsonize << AppendFileName(folder, file);
		}*/
		for (int i = 0; i < left2.pending.GetCount(); ++i) {
			String file = left2.pending.Get(i, pend(idFile));
			String folder = left2.pending.Get(i, pend(idFolder));
			pendingToJsonize << AppendFileName(folder, file);
		}
		for (int i = 0; i < left3.done.GetCount(); ++i)
			doneData << left3.done.GetLine(i);
	}
	json
		("maxCPU", 		left3.maxCPU)
		("newCPU", 		left2.pendCPU)
		("newTime", 	left2.pendTime)
		("pending", 	pendingToJsonize)
		("done", 		doneData)
	;
	if (json.IsLoading()) {
		for (int i = 0; i < doneData.GetCount(); ++i)
			left3.done.Set(i, doneData[i]);
		UpdateLabs();
	}
}

bool Main::Init() {
	String manufacturer, productName, version, mbSerial;
	int numberOfProcessors; 
	GetSystemInfo(manufacturer, productName, version, numberOfProcessors, mbSerial);	
	   
	if (IsNull(~left3.maxCPU)) 
		left3.maxCPU <<= numberOfProcessors;
	left3.maxCPU.MinMax(1, numberOfProcessors);
	
	left3.maxSimult <<= 0;
	left3.maxSimult.MinMax(0, ~left3.maxCPU);
	left3.maxSimult.Step(1, true);
	left3.maxSimult.SetMajorTicks(int(numberOfProcessors/6)).SetMajorTicksSize(4).SetThickness(2);
	left3.maxSimult.WhenAction = [&] {left3.maxSimultVal <<= ~left3.maxSimult;};
	left3.maxSimult.WhenAction();
	
	if (IsNull(~left2.pendTime))
		left2.pendTime <<= "24:00:00";
	
	OnCPUSpin();
	
	procid = 0;
	
	if (pendingToJsonize.GetCount() > 0) {
		if (PromptYesNo(Format(t_("There are %d pending processes from previous execution. Do you want to run them?"), pendingToJsonize.GetCount()))) {
			for (int i = 0; i < pendingToJsonize.GetCount(); ++i)
				DoDrop(pendingToJsonize[i]);
		}
		pendingToJsonize.Clear();
	}
		
	return true;
}

void Main::Start() {
	timeCallback.Set(-500, THISBACK(TimerFun));
}

void Main::Stop() {
	timeCallback.Kill();
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

void Main::ClearRealized() {
	left3.done.Clear();
	UpdateLabs();
}
		
void Main::DnD(PasteClip& d) {
	if (AcceptText(d)) 
		DoDrop(GetString(d));
}

void Main::DnDInsertProc(int line, PasteClip& d) {
	if(AcceptInternal<ArrayCtrl>(d, "array")) {
		const ArrayCtrl &array = GetInternal<ArrayCtrl>(d);
		if (&array != &left1.processing) 
			d.Reject();
		else {
			left1.processing.InsertDrop(line, d);
			left1.processing.SetFocus();
			UpdateLabs();
		}
	}
	if(AcceptText(d)) {
		left1.processing.Insert(line);
		UpdateLabs();
		left1.processing.Set(line, pend(idId), GetString(d));
		left1.processing.SetCursor(line);
		left1.processing.SetFocus();
	}
}

void Main::DragProc() {
	if (left1.processing.DoDragAndDrop(InternalClip(left1.processing, "array")) == DND_MOVE) {
		left1.processing.RemoveSelection();
		UpdateLabs();
	}
}

Vector<Value> Main::DoneToPend(const Vector<Value> &linedone) {
	Vector<Value> linepend(8);
	linepend[pend(idId)] = linedone[done(idId)];
	linepend[pend(idFile)] = linedone[done(idFile)];
	linepend[pend(idArgs)] = linedone[done(idArgs)];
	linepend[pend(idFolder)] = linedone[done(idFolder)];
	linepend[pend(idUser)] = linedone[done(idUser)];
	linepend[pend(idComputer)] = linedone[done(idComputer)];
	linepend[pend(idCPU)] = ~left2.pendCPU;
	linepend[pend(idTime)] = linedone[done(idTime)];	
	return linepend;
}

void Main::DnDInsertPend(int line, PasteClip& d) {
	if (AcceptInternal<ArrayCtrl>(d, "array")) {
		const ArrayCtrl &array = GetInternal<ArrayCtrl>(d);
		if (&array == &left3.done) {
		 	Vector<Vector<Value>> data;
			for(int i = 0; i < array.GetCount(); i++) {
				if(array.IsSel(i))
					data.Add(DoneToPend(array.GetLine(i)));
			}
		 	left2.pending.Insert(line, data);
		 	left2.pending.SetCursor(line);
		} else
			left2.pending.InsertDrop(line, d);
		left2.pending.SetFocus();
		UpdateLabs();
	} 
	if(AcceptText(d)) {
		left2.pending.Insert(line);
		UpdateLabs();
		left2.pending.Set(line, pend(idId), GetString(d));
		left2.pending.SetCursor(line);
		left2.pending.SetFocus();
	}
}

void Main::DragPend() {
	if(left2.pending.DoDragAndDrop(InternalClip(left2.pending, "array")) == DND_MOVE) {
		left2.pending.RemoveSelection();
		UpdateLabs();
	}
}

void Main::DragAndDrop(Point p, PasteClip& d) {
	if (IsDragAndDropSource())
		return;
	if (AcceptFiles(d)) {
		Vector<String> files = GetFiles(d);
		for (int i = 0; i < files.GetCount(); ++i)
			DoDrop(files[i]);
		Refresh();
	}
}

void Main::DragDone() {
	if (left3.done.DoDragAndDrop(InternalClip(left3.done, "array")) == DND_MOVE) {
		left3.done.RemoveSelection();
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
	for (int i = 0; i < batches.GetCount(); ++i) {
		if (batches[i].IsShown()) {// && batches[i].process.IsRunning()) {
			String str = WString((wchar *)&key, 1).ToString();
			batches[i].process.Write(str);
			break;
		}
	}
	return true;
}

void Main::DoDrop(String name) {
	/*if (DirectoryExists(name)) {
		String strExe = AppendFileName(name, "SENERWave_exe.exe");
		if (FileExists(strExe))
			name = strExe;
		else
			return;
	}*/
	if (GetFileExt(name) != ".bat" && GetFileExt(name) != ".exe" && GetFileExt(name) != ".dat") 
		return;
	if (GetFileExt(name) == ".bat") {
		String str = LoadFile(name);
		if (str.IsEmpty()) 
			if (!PromptOKCancel(Format(t_("File %s is empty.&") + String(t_("Do you want to continue?")), DeQtf(name))))
				return;
		if (ToLower(str).Find("pause") >= 0) {
			if (!PromptOKCancel(Format(t_("File %s has a PAUSE command.&") + String(t_("Do you want to continue?")), DeQtf(name))))
				return;
		}
	}
	String args;

	String folder = GetFileFolder(name);
	
	if (ToLower(name).EndsWith(".dat")) {
		args = Format("-orca -numtries 10 -timelog 10 -rf \"%s\" \"%s\"", name, ForceExt(name, ".sim"));
		name = "bemrosetta_cl";
	} 

	left2.pending.Add(procid, GetFileName(name), args, folder, GetUserName(), GetComputerName(), ~left2.pendCPU, ~left2.pendTime);
	UpdateLabs();
	
	Batch &batch = batches.Add();
	right.batchesArea.Add(batch.SizePos());
	
	batch.batId 	  = procid;
	batch.id 	    <<= procid;
	batch.file 		<<= GetFileName(name);
	batch.folder 	<<= folder;
	batch.args 		<<= args;
	batch.maxCPU 	<<= ~left2.pendCPU;	
	batch.maxTime	<<= ~left2.pendTime;	
	batch.user		<<= GetUserName();	
	batch.comp		<<= GetComputerName();	
	batch.begin 	<<= "-";	
	batch.butPause.Enable(false);
	batch.butStop.Enable(false);
	
	left2.pending.SetCursor(left2.pending.GetCount() - 1);
	left2.pending.SetFocus();
	
	procid++;
	
	//Save();
}

void Main::OnPendingsDelete() {
	int row = left2.pending.GetCursor();
	if (row < 0) 
		return;
	if (!PromptYesNo(t_("Do you want to remove selected task?")))
		return;
	
	left2.pending.Remove(row);
	UpdateLabs();
}

Vector<int> GetSelectedRows(const ArrayCtrl &array) {
	Vector<int> ret;
	for(int row = 0; row < array.GetCount(); row++)
		if(array.IsSelected(row))
			ret << row;
	return ret;
}

void Main::OnPendingsCPU() {
	ArrayCtrl &array = selected == 0 ? left1.processing : left2.pending;
	IdsArray    &ids = selected == 0 ? proc : pend;
	
	Vector<int> sels = GetSelectedRows(array);
	if (sels.IsEmpty()) 
		return;

	for (int i = 0; i < sels.size(); ++i) {
		int row = sels[i];
		int id = array.Get(row, ids(idId));
		array.Set(row, ids(idCPU), ~left2.pendCPU);	
		batches[id].maxCPU = ~left2.pendCPU;		
	}
}

void Main::OnPendingsTime() {
	ArrayCtrl &array = selected == 0 ? left1.processing : left2.pending;
	IdsArray    &ids = selected == 0 ? proc : pend;
	
	Vector<int> sels = GetSelectedRows(array);
	if (sels.IsEmpty()) 
		return;

	for (int i = 0; i < sels.size(); ++i) {
		int row = sels[i];
		int id = array.Get(row, ids(idId)); 
		String str = ~left2.pendTime;
		if (selected != 0)
			array.Set(row, ids(idTime), str);	
		else {
			double d = StringToSeconds(str);
			if (!IsNull(d) && d > batches[id].process.Seconds() + 60)	// To avoid stop a process when editing a new timeout
				batches[id].process.SetMaxRunTime(d);
		}
		batches[id].maxTime <<= ~left2.pendTime;	
	}
}		   

void Main::OnMove(int delta, int processingrow) {	// processingrow forces processing ans row
	ArrayCtrl &array = selected == 0 || processingrow >= 0 ? left1.processing : left2.pending;
	IdsArray    &ids = selected == 0 || processingrow >= 0 ? proc : pend;
	
	Vector<int> idss;		// Unique identifiers
	if (processingrow >= 0) {
		int id = array.Get(processingrow, ids(idId)); 	
		idss << id;
	} else {
		Vector<int> sels = GetSelectedRows(array);
		if (sels.IsEmpty()) 
			return;
	
		for (int i = 0; i < sels.size(); ++i) {
			int id = array.Get(sels[i], ids(idId)); 	
			idss << id;
		}
	}
	if (delta == -10000)
		Reverse(idss);
	
	for (int i = 0; i < idss.size(); ++i) {
		for (int row = 0; row < array.GetCount(); ++row) {
			int id = array.Get(row, ids(idId)); 
			if (id == idss[i]) {
				int ddelta = delta;
				if (row + delta < 0)
					ddelta = -row;
				else if (row + delta >= array.GetCount())
					ddelta = array.GetCount() - row - 1;
				
				if (ddelta != 0) {
					array.SetCursor(row);
					int dd = abs(ddelta);
					ddelta = ddelta/dd;
					for (; dd > 0; --dd)
						array.Move(ddelta);
				}
				break;
			}
		}
	}
	for (int row = 0; row < array.GetCount(); ++row) {
		int id = array.Get(row, ids(idId)); 
		if (Find(idss, id) >= 0)
			array.Select(row);
	}
}

void Main::Close() { 
	DoClose(true);	
}

void Main::DoClose(bool prompt) {
	pendingToJsonize.Clear();
	int i;
	for (i = 0; i < batches.GetCount(); ++i)
		if (batches[i].process.IsRunning()) 
			break;
	if (i < batches.GetCount() && prompt) {	
		if(!PromptYesNo(t_("There are processes opened.&Do you want to end BatchManager?")))
			return;
	
		if(PromptYesNo(t_("Do you want to cancel all processes?"))) {
			for (i = 0; i < batches.GetCount(); ++i) {
				if (batches[i].process.IsRunning()) {
					batches[i].Stop();
					String folder = ~batches[i].folder;
					String file = ~batches[i].file;
					pendingToJsonize << AppendFileName(folder, file);
				}
			}
		}
	}
	
	RejectBreak(IDOK);		// Empty EditStrings does not disturb
	//MessageWindow::CloseAll();
	TopWindow::Close();	
}

void Main::OnSel(int who) {
	NON_REENTRANT_V;

	selected = who;
	
	ArrayCtrl *grid;
	switch (who) {
	case 0:	grid = &left1.processing;
			left3.done.KillCursor();
			left2.pending.KillCursor();
			break;
	case 1:	grid = &left3.done;
			left1.processing.KillCursor();
			left2.pending.KillCursor();
			break;
	case 2:	grid = &left2.pending;
			left3.done.KillCursor();
			left1.processing.KillCursor();
			break;
	default:return;
	}
	int row = grid->GetCursor();
	if (row < 0) 
		return;
	
	if (who == 2)
		left2.pendCPU <<= left2.pending.Get(row, pend(idCPU));
	
	left2.butTopPending.Enable(who != 1);
	left2.butUpPending.Enable(who != 1);
	left2.butDownPending.Enable(who != 1);
	left2.butBottomPending.Enable(who != 1);
	
	int id = grid->Get(row, 0);
	for (int i = 0; i < batches.GetCount(); ++i) {
		//int batId = batches[i].batId;
		batches[i].Show(id == batches[i].batId);
	}
}

String FormatBytes(uint64 bytes) {
    static const char* suffixes[] = { "b", "kb", "Mb", "Gb", "Tb" };
    int suffixIndex = 0;
    double size = bytes;

    while (size >= 1024 && suffixIndex < 4) {
        size /= 1024;
        suffixIndex++;
    }
    return Format("%.1f %s", size, suffixes[suffixIndex]);
}

void Main::TimerFun() {
	NON_REENTRANT_V;
	
	calcMemCounter++;
	
	if (calcMemCounter == 1 || !(calcMemCounter%5)) {		// Each 5 times
		uint64 total, available;
		GetSystemMemoryStatus(total, available);
		const double btoGb = 1024*1024*1024;
		double dtotal = total/btoGb;
		double dused = (total-available)/btoGb;
		left3.memTotal = Format("%.1f", dtotal);
		left3.memUsed  = Format("%.1f", dused);
		double ratioMem = dused/dtotal;
		Color col;
		if (ratioMem < 0.7)
			col = LtGreen();
		else if (ratioMem < 0.9)
			col = Yellow();
		else
			col = LtRed();
		left3.semaphore.SetBackground(col);
	}
	
	int cpuAvailables = ~left3.maxSimult;
	int numRunProcess = 0;
	for (int i = 0; i < left1.processing.GetCount(); ++i) {
		cpuAvailables -= int(left1.processing.Get(i, proc(idCPU)));
		String file = left1.processing.Get(i, proc(idFile));
		String status = left1.processing.Get(i, proc(idStatus));
		if (status == t_("Running"))
			numRunProcess += int(left1.processing.Get(i, proc(idCPU)));
	}
	
	// Any has to be stopped?
	for (int i = 0; i < batches.GetCount(); ++i) {
		Batch &batch = batches[i];
		if (batch.isEnded || !batch.isStarted)
			continue;
		
		int processingRow = -1;
		for (int i = 0; i < left1.processing.GetCount(); ++i) {
			if (batch.batId == left1.processing.Get(i, proc(idId))) {
				processingRow = i;
				break;
			}
		}
		if (processingRow < 0)
			throw Exc("Processing row not found");
		
		if (batch.process.IsPaused()) 
			left1.processing.Set(processingRow, proc(idStatus), t_("Paused"));	
		else 
			left1.processing.Set(processingRow, proc(idStatus), t_("Running"));
		
		if (batch.process.GetData() == t_("Process is PAUSED")) {
			batch.process.SetData("");
			OnMove(10000, processingRow);
			if (numRunProcess-1 < int(~left3.maxSimult))
				left3.maxSimult <<= numRunProcess-1;
			return;
		} else if (batch.process.GetData() == t_("Process is RUNNING")) {
			batch.process.SetData("");
			OnMove(-10000, processingRow);
			return;
		}
		
		batch.process.Perform();
		
		if (batch.process.IsRunning()) {
			if (!(calcMemCounter%5)) {		// Each 5 times
				left1.processing.Set(processingRow, proc(idTime), SecondsToString(batch.process.Seconds(), 0, false, false, true, false, true) + "/" + 
													   SecondsToString(batch.process.GetMaxRunTime(), 0, false, false, true, false, true));
				uint64 memory = batch.process.GetMemory();
				if (!batch.process.IsPaused()) {
					batch.sumMem += memory;
					batch.numMem++;
				}
				left1.processing.Set(processingRow, proc(idMem), FormatBytes(memory) + "/" + FormatBytes(batch.sumMem/batch.numMem));
			}
		} else {
			batch.butStop.Enable(false);
			batch.butPause.Enable(false);
			batch.isEnded = true;
			
			String msg;
			if (batch.process.GetData() == t_("Continue later")) {
				batch.process.SetData("");
				msg = t_("Program temporarily ended. It will restart later when a thread is available");
				
				DoDrop(AppendFileNameX(~batch.folder, ~batch.file));
			} else {
				switch(batch.process.GetStatus()) {
				case LocalProcessX::RUNNING:	 	break;
				case LocalProcessX::STOP: 	 	 	msg = Format(t_("Program ended. Exit code %d"), batch.process.GetExitCode());	break;
				case LocalProcessX::STOP_USER: 	 	msg = t_("Program stopped by user");			break;
				case LocalProcessX::STOP_TIMEOUT:	msg = t_("Execution time exceeded (timeout)");	break;
				case LocalProcessX::STOP_NORESPONSE:msg = t_("Application does not respond");		break;	
				default:						 	throw Exc(Format("Unknown process status %d", batch.process.GetStatus()));
				}
			}
			left3.done.Insert(0);				// Adds at the beginning
			left3.done.Set(0, done(idId), batch.batId);
			left3.done.Set(0, done(idFile), ~batch.file);
			left3.done.Set(0, done(idArgs), ~batch.args);
			left3.done.Set(0, done(idFolder), ~batch.folder);
			left3.done.Set(0, done(idBegin), ~batch.begin);
			left3.done.Set(0, done(idEnd), GetSysTime());
			left3.done.Set(0, done(idDuration), Format("%s (%d)", SecondsToString(batch.process.Seconds(), 0, false, false, true, false, true), batch.process.Seconds()));
			left3.done.Set(0, done(idUser), ~batch.user);
			left3.done.Set(0, done(idComputer), ~batch.comp);
			left3.done.Set(0, done(idTime), SecondsToString(batch.process.GetMaxRunTime(), 0, false, false, true, false, true));
			String str;
			if (batch.numMem == 0)
				str = "-";
			else {
				uint64 bytes = batch.sumMem/batch.numMem;
				str = Format("%s (%s)", FormatBytes(bytes), Format64(bytes));
			}
			left3.done.Set(0, done(idMemory), str);
				
			left1.processing.Remove(processingRow);
			
			batch.output.Print("\n" + msg + "\n", Green());	
			
			UpdateLabs();
			
			Save();
			
			return;
		}
	}
	// Update screen?
	for (int i = 0; i < batches.size(); ++i) {
		if (batches[i].IsVisible()) {
			Batch &batch = batches[i];
			if (!batch.process.IsRunning())
				break;
		}
	}
	/*
	int cpuAvailables = ~left3.maxSimult;
	int numRunProcess = 0;
	for (int i = 0; i < left1.processing.GetCount(); ++i) {
		cpuAvailables -= int(left1.processing.Get(i, 5));
		String file = left1.processing.Get(i, 1);
		String status = left1.processing.Get(i, 8);
		if (status == t_("Running"))
			numRunProcess += int(left1.processing.Get(i, 5));
	}*/
	
	UpdateLabs();
	
	// Pauses paused processes, if no CPUs available
	if (int(~left3.maxSimult) < numRunProcess) {
		for (int i = left1.processing.GetCount()-1; i >= 0; --i) {		// From first to last, to avoid pausing processes that have stayed longer
			if (left1.processing.Get(i, proc(idStatus)) != t_("Paused")) {
				int id = left1.processing.Get(i, proc(idId));
				batches[id].OnPause();
				return;		
			}	
		}
		return;
	}

	// Unpauses paused processes, if CPUs are available
	for (int i = 0; i < left1.processing.GetCount(); ++i) {
		if (left1.processing.Get(i, proc(idStatus)) == t_("Paused") &&
			int(left1.processing.Get(i, proc(idCPU))) <= int(~left3.maxSimult) - numRunProcess) {
			int id = left1.processing.Get(i, proc(idId));
			batches[id].OnPause();
			return;		
		}
	}

	if (cpuAvailables <= 0)
		return;
			
	int idPendings;
	for (idPendings = 0; idPendings < left2.pending.GetCount(); ++idPendings) {
		int numCPU = left2.pending.Get(idPendings, pend(idCPU));	
		if (numCPU <= cpuAvailables) 
			break;
	}
	if (idPendings >= left2.pending.GetCount())	// There are no processes to add
		return;

	int id;
	int batId = left2.pending.Get(idPendings, pend(idId));
	for (id = 0; id < batches.size(); ++id) {
		if (batches[id].batId == batId)
			break;
	}
	if (id >= batches.size())
		return;
	Batch &batch = batches[id];

	batch.begin <<= GetSysTime();	
	batch.butPause.Enable(true);
	batch.butStop.Enable(true);
	String file 	= ~batch.file;
	
	left2.pending.Remove(idPendings);
	
	left1.processing.Add(batch.batId, ~batch.file, ~batch.args, ~batch.folder, 
						~batch.user, ~batch.comp, ~batch.maxCPU,
						~batch.begin, ~batch.maxTime, "", t_("Running"));
	left1.processing.SetCursor(left1.processing.GetCount() - 1);

	
	UpdateLabs();
	
	//Vector<String> args;
	//args << ~batch.file << ~batch.args << ~batch.folder << ~batch.begin << FormatInt(~batch.maxCPU) 
	//	 << ~batch.user << ~batch.comp << ~batch.maxTime;					

	String folder = ~batch.folder;
	String driveLetter = "";
	if (folder[1] == ':')
		driveLetter = folder.Left(2) + " && ";
	String command;
	if (GetFileExt(file) == ".bat") 
		command = String("cmd.exe /c \"") + driveLetter + String("cd \"") + folder + "\" && \"" + String(~batch.file) + "\"\"";
	else {
		command = AppendFileName(String(~batch.folder), String(~batch.file)); 
		command << " " << ~batch.args;
	}
	int maxTime = StringToSeconds(String(~batch.maxTime));
	batch.process.Start(command, nullptr, nullptr, -1, maxTime, maxTime);
	batch.isStarted = true;
	batch.isEnded = false;
	OnSel(0);
}	
