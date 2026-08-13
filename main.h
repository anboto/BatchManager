// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 - 2026, the BatchManager author
#ifndef _BatchManager_main_h_
#define _BatchManager_main_h_

#include <CtrlLib/CtrlLib.h>
#include <SysInfo/SysInfo.h>
#include <Controls4U/Controls4U.h>
#include <ScatterCtrl/ScatterCtrl.h>

using namespace Upp;


#define LAYOUTFILE <BatchManager/BatchManager.lay>
#include <CtrlCore/lay.h>

String FormatBytes(uint64 bytes);
void AddToPATH(const Vector<String>& addFolders);
	
class Main;

class Task : public StaticRect {
typedef Task CLASSNAME;
public:
	Task();
	virtual ~Task() {}
	
	void Start(String folder, String file, String args, double maxTime, const Vector<String> &path);
	void Perform(bool updateScatter, bool forceZoom, bool updateSeries);
	
	void OnStop();
	
	bool OnProcess(double elapsed, const String &out, bool isEnd, bool &resetTimeout);
	
	int id;
	uint64 memMax = 0, memTotal = 0, memRam = 0;
	int paging_s;
	
	LocalProcessX process;
	Time lastPausedStarted = Null;
	
	Vector<int> vtime;
	Vector<int> vtotal, vram;
	Vector<int> vpaging_s;
	
	void LoadNew();
	void LoadLoaded();
	
	Task &SetParent(Main &_parent)	{parent = &_parent;	return *this;}
	
	void Jsonize(JsonIO& json);
	
	void UpdateScatter(bool forceZoom = false);
	
	ConsoleText console;
	ScatterCtrl scatter;
	Box splitter;
	
private:
	friend class Batch;
	
	Main *parent = nullptr;
};

class Batch {
typedef Batch CLASSNAME;
public:
	virtual ~Batch() {}

	Task &Add(int id, Main &parent) {
		Task &task = tasks.Add();
		task.id = id;
		task.parent = &parent;
		return task;
	}
	void Remove(int id)   {tasks.Remove(GetIndex(id));}
	Task &GetId(int id)   {return tasks[GetIndex(id)];}
	Task &GetIdx(int idx) {return tasks[idx];}
	int size()			  {return tasks.size();}
	void Clear()		  {tasks.Clear();}
	
	int GetShown() {
		for (int i = 0; i < tasks.size(); ++i) {
			if (tasks[i].IsShown())
				return i;
		}
		return -1;
	}
	void Show(int id) {
		for (Task &task : tasks)
			task.Show(id == task.id);
	}
	bool IsRunning() {
		for (Task &task : tasks) {
			if (task.process.IsRunning()) 
				return true;
		}
		return false;
	}
	void Stop() {
		for (Task &task : tasks)
			if (task.process.IsRunning())
				task.process.Stop();
	}
	int MaxId() {
		int id = 0;
		for (Task &task : tasks)
			id = max(id, task.id);
		return id;
	}
	String GetConsole(int id);
	void SetConsole(int id, String str);
	
	void Jsonize(JsonIO& json);
		
private:
	Array<Task> tasks;	
	
	int GetIndex(int id);
};

struct HandledTypes {
	String pattern, command, args;
	
	void Jsonize(JsonIO& json) {
		json
			("pattern",	pattern)
			("command", command)
			("args", 	args)
		;
	}
};

class Main : public TopWindow {
typedef Main CLASSNAME;
public:
	Main();
	virtual ~Main();

	void Load();
	void Save();
	void Clear();
	
	void OnSel();
	void TimerFun();
	int RunCommand(const char *cmd);
	virtual void Close();
	void DoClose(bool prompt);
	void Jsonize(JsonIO& json);
	bool Init();
	void StartCallBack();
	void StopCallBack();
	void TaskCount(bool onlySelected, int &pending, int &paused, int &running, int &ended);
		
	SplitterButton splitter, splitterH;
	WithMain<StaticRect> main;
	WithOutput<StaticRect> output;
	
	void ClearRealized();
	
	void UpdateScatters();
	
private:
	void OnDropInsert(int line, PasteClip& d);
	void OnDrag();
	virtual void DragAndDrop(Point p, PasteClip& d);
	bool Key(dword key, int count);
	void DoDrop(String name, String args = "");	
	
	void OnCPUSpin();
	void OnRAMSpin();
	void OnReStart();
	void OnEnd();
	void OnDelete();
	void OnMaxTime();
	void OnMove(int delta, int row);
	void OnFolder();
	void OnSort();
	void OnHandledTypes();
	void OnSetPath();
	void OnHelp();
	  
	void InitButtons();
	void UpdateLabs();
	
	Index<int> GetSelectedRows();
	Index<int> GetSelectedIds();
	void SetSelectedIds(const Index<int> &ids);
	
	int idId, idStatus, idFile, idArgs, idFolder, idUser, idComputer, idStart, idEnd, idMaxTime, idTime, idMemActual, idMemMax;
	
	Index<String> status;
	
	Batch batch;
		
	String configFile;
	
	int procid;
	
	double jsonVersion = Null;
	
	Array<HandledTypes> handledTypes;
	Vector<String> path;
	
	TimeCallback timeCallback;
	Time lastScatter, lastSeries, lastSerialize;
};

class RightDisplay : public Display {
public:
    virtual void Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const override {
        String text = q.ToString();
        w.DrawRect(r, paper);
        Size sz = GetTextSize(text, StdFont());
        int x = r.right - sz.cx - 2;                 	// Horizontally right, margin 2 . Left would be int x = r.left + 2;  
        int y = r.top + (r.Height() - sz.cy) / 2;     	// Vertically centered
        w.DrawText(x, y, text, StdFont(), ink);
    }
};

class GreenRunningDisplay : public Display {
public:
    virtual void Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const override {
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
        w.DrawText(x, y, text, StdFont(), textColor);	// Text color forced to Black
    }
};
		
class DialogHandledTypes : public WithHandledTypes<TopWindow> {
public:
    typedef DialogHandledTypes CLASSNAME;

    EditString editPattern, editCommand, editArgs;

    DialogHandledTypes() {
        CtrlLayout(*this);
        
        Title(t_("Managed file types")).Sizeable().Zoomable();

        array.AddColumn("Pattern", t_("Pattern"),   2).Edit(editPattern).HeaderTab().SetMargin(0);
        array.AddColumn("Command", t_("Command"),   2).Edit(editCommand).HeaderTab().SetMargin(0);
        array.AddColumn("Args",    t_("Arguments"), 8).Edit(editArgs).HeaderTab().SetMargin(0);
        array.SetLineCy(int(1.4*StdFont().GetCy()));

        array.WhenBar = THISBACK(ArrayMenu);

        butOK    << [=]{Break(IDOK);};
        butCancel<< [=]{Break(IDCANCEL);};
    }
    void ArrayMenu(Bar& bar) {
        bar.Add(				  "Add row",    [=] {array.Add(); array.SetCursor(array.GetCount() - 1);});
        bar.Add(array.IsCursor(), "Remove row", [=] {array.Remove(array.GetCursor());});
    }
    void Load(const Array<HandledTypes>& data) {
        array.Clear();
        for(const HandledTypes& h : data) {
            array.Add();
            int r = array.GetCount() - 1;
            array.Set(r, "Pattern", h.pattern);
            array.Set(r, "Command", h.command);
            array.Set(r, "Args", h.args);
        }
    }
    void Save(Array<HandledTypes>& data) const {
        data.Clear();
        for(int row = 0; row < array.GetCount(); row++) {
            HandledTypes& h = data.Add();
            h.pattern = array.Get(row, "Pattern");
            h.command = array.Get(row, "Command");
            h.args    = array.Get(row, "Args");
        }
    }	
};

class DialogPath : public WithPath<TopWindow> {
public:
    typedef DialogPath CLASSNAME;

    EditFolder editFolder;

    DialogPath() {
        CtrlLayout(*this);
        
        Title("Add folders to PATH").Sizeable().Zoomable();

        array.AddColumn("Folder", t_("Folder")).Edit(editFolder).HeaderTab().SetMargin(0);
        array.SetLineCy(int(1.4*StdFont().GetCy()));

        array.WhenBar = THISBACK(ArrayMenu);

        butOK    << [=]{Break(IDOK);};
        butCancel<< [=]{Break(IDCANCEL);};
    }
    void ArrayMenu(Bar& bar) {
        bar.Add(				  "Add row",    [=] {array.Add(); array.SetCursor(array.GetCount() - 1);});
        bar.Add(array.IsCursor(), "Remove row", [=] {array.Remove(array.GetCursor());});
    }
    void Load(const Vector<String>& data) {
        array.Clear();
        for(const String& h : data) {
            array.Add();
            int r = array.GetCount() - 1;
            array.Set(r, "Folder", h);
        }
    }
    void Save(Vector<String>& data) const {
        data.Clear();
        for(int row = 0; row < array.GetCount(); row++) {
            String& h = data.Add();
            h = array.Get(row, "Folder");
        }
    }	
};

#endif
