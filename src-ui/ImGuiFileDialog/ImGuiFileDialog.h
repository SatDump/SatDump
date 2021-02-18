/*
MIT License

Copyright (c) 2019-2020 Stephane Cuillerdier (aka aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
-----------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------------

github repo : https://github.com/aiekick/ImGuiFileDialog

-----------------------------------------------------------------------------------------------------------------
## Description :
-----------------------------------------------------------------------------------------------------------------

this File Dialog is build on top of DearImGui
(On windows, need te lib Dirent : https://github.com/tronkko/dirent, use the branch 1.23 for avoid any issues)
Complete readme here : https://github.com/aiekick/ImGuiFileDialog/blob/master/README.md)

this filedialog was created principally for have custom pane with widgets accrdoing to file extention.
it was not possible with native filedialog

An example of the File Dialog integrated within the ImGui Demo App

-----------------------------------------------------------------------------------------------------------------
## Features :
-----------------------------------------------------------------------------------------------------------------

- Separate system for call and display
  - can be many func calls with different params for one display func by ex
- Can use custom pane via function binding
  - this pane can block the validation of the dialog
  - can also display different things according to current filter and User Datas
- Support of Filter Custom Coloring / Icons / text
- Multi Selection (ctrl/shift + click) :
  - 0 => infinite
  - 1 => one file (default)
  - n => n files
- Compatible with MacOs, Linux, Win
  - On Win version you can list Drives
- Support of Modal/Standard dialog type
- Support both Mode : File Chooser or Directory Chooser
- Support filter collection / Custom filter name
- Support files Exploring with keys : Up / Down / Enter (open dir) / Backspace (come back)
- Support files Exploring by input char (case insensitive)
- Support bookmark creation/edition/call for directory (can have custom name corresponding to a path)
- Support input path edition by right click on a path button
- Support of a 'Confirm to Overwrite" dialog if File Exist


-----------------------------------------------------------------------------------------------------------------
## NameSpace / SingleTon
-----------------------------------------------------------------------------------------------------------------

Use the Namespace IGFD (for avoid conflict with variables, struct and class names)

you can display only one dialog at a time, this class is a simgleton and must be called like that :
ImGuiFileDialog::Instance()->method_of_your_choice()

-----------------------------------------------------------------------------------------------------------------
## Simple Dialog :
-----------------------------------------------------------------------------------------------------------------

Example code :
void drawGui()
{
  // open Dialog Simple
  if (ImGui::Button("Open File Dialog"))
	ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".cpp,.h,.hpp", ".");

  // display
  if (ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey"))
  {
	// action if OK
	if (ImGuiFileDialog::Instance()->IsOk == true)
	{
	  std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
	  std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
	  // action
	}
	// close
	ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey");
  }
}

-----------------------------------------------------------------------------------------------------------------
## Directory Chooser :
-----------------------------------------------------------------------------------------------------------------

For have only a directory chooser, you just need to specify a filter null :

Example code :
ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose a Directory", 0, ".");

In this mode you can select any directory with one click, and open directory with double click

-----------------------------------------------------------------------------------------------------------------
## Dialog with Custom Pane :
-----------------------------------------------------------------------------------------------------------------

Example code :
static bool canValidateDialog = false;
inline void InfosPane(std::string& vFilter, IGFD::UserDatas vUserDatas, bool *vCantContinue) // if vCantContinue is false, the user cant validate the dialog
{
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Infos Pane");
	ImGui::Text("Selected Filter : %s", vFilter.c_str());
	if (vUserDatas)
		ImGui::Text("UserDatas : %s", vUserDatas);
	ImGui::Checkbox("if not checked you cant validate the dialog", &canValidateDialog);
	if (vCantContinue)
		*vCantContinue = canValidateDialog;
}

void drawGui()
{
  // open Dialog with Pane
  if (ImGui::Button("Open File Dialog with a custom pane"))
	ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".cpp,.h,.hpp",
			".", "", std::bind(&InfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 350, 1, IGFD::UserDatas("InfosPane"));

  // display and action if ok
  if (ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey"))
  {
	if (ImGuiFileDialog::Instance()->IsOk == true)
	{
		std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
		std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
		std::string filter = ImGuiFileDialog::Instance()->GetCurrentFilter();
		// here convert from string because a string was passed as a userDatas, but it can be what you want
		std::string userDatas;
		if (ImGuiFileDialog::Instance()->GetUserDatas())
			userDatas = std::string((const char*)ImGuiFileDialog::Instance()->GetUserDatas());
		auto selection = ImGuiFileDialog::Instance()->GetSelection(); // multiselection

		// action
	}
	// close
	ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey");
  }
}

-----------------------------------------------------------------------------------------------------------------
## Filter Infos
-----------------------------------------------------------------------------------------------------------------

You can define color for a filter type
Example code :
ImGuiFileDialog::Instance()->SetExtentionInfos(".cpp", ImVec4(1,1,0, 0.9));
ImGuiFileDialog::Instance()->SetExtentionInfos(".h", ImVec4(0,1,0, 0.9));
ImGuiFileDialog::Instance()->SetExtentionInfos(".hpp", ImVec4(0,0,1, 0.9));
ImGuiFileDialog::Instance()->SetExtentionInfos(".md", ImVec4(1,0,1, 0.9));


![alt text](doc/color_filter.png)

and also specific icons (with icon font files) or file type names :

Example code :
// add an icon for png files
ImGuiFileDialog::Instance()->SetExtentionInfos(".png", ImVec4(0,1,1,0.9), ICON_IMFDLG_FILE_TYPE_PIC);
// add a text for gif files (the default value is [File]
ImGuiFileDialog::Instance()->SetExtentionInfos(".gif", ImVec4(0, 1, 0.5, 0.9), "[GIF]");


![alt text](doc/filter_Icon.png)

-----------------------------------------------------------------------------------------------------------------
## Filter Collections
-----------------------------------------------------------------------------------------------------------------

you can define a custom filter name who correspond to a group of filter

you must use this syntax : custom_name1{filter1,filter2,filter3},custom_name2{filter1,filter2},filter1
when you will select custom_name1, the gorup of filter 1 to 3 will be applied
the reserved char are {}, you cant use them for define filter name.

Example code :
const char *filters = "Source files (*.cpp *.h *.hpp){.cpp,.h,.hpp},Image files (*.png *.gif *.jpg *.jpeg){.png,.gif,.jpg,.jpeg},.md";
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", ICON_IMFDLG_FOLDER_OPEN " Choose a File", filters, ".");

## Multi Selection

You can define in OpenDialog/OpenModal call the count file you wan to select :
- 0 => infinite
- 1 => one file only (default)
- n => n files only

See the define at the end of these funcs after path.

Example code :
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".*,.cpp,.h,.hpp", ".");
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose 1 File", ".*,.cpp,.h,.hpp", ".", 1);
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose 5 File", ".*,.cpp,.h,.hpp", ".", 5);
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose many File", ".*,.cpp,.h,.hpp", ".", 0);
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".png,.jpg",
   ".", "", std::bind(&InfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 350, 1, "SaveFile"); // 1 file

-----------------------------------------------------------------------------------------------------------------
## File Dialog Constraints
-----------------------------------------------------------------------------------------------------------------

you can define min/max size of the dialog when you display It

by ex :

* MaxSize is the full display size
* MinSize in the half display size.

Example code :
ImVec2 maxSize = ImVec2((float)display_w, (float)display_h);
ImVec2 minSize = maxSize * 0.5f;
ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, minSize, maxSize);

-----------------------------------------------------------------------------------------------------------------
## Detail View Mode
-----------------------------------------------------------------------------------------------------------------

You can have tables display like that.

- uncomment "#define USE_IMGUI_TABLES" in you custom config file (CustomImGuiFileDialogConfig.h in this example)

-----------------------------------------------------------------------------------------------------------------
## Exploring by keys
-----------------------------------------------------------------------------------------------------------------

you can activate this feature by uncomment : "#define USE_EXPLORATION_BY_KEYS"
in you custom config file (CustomImGuiFileDialogConfig.h in this example)

you can also uncomment the next lines for define your keys :

* IGFD_KEY_UP => Up key for explore to the top
* IGFD_KEY_DOWN => Down key for explore to the bottom
* IGFD_KEY_ENTER => Enter key for open directory
* IGFD_KEY_BACKSPACE => BackSpace for comming back to the last directory

you can also explore a file list by use the current key char.

as you see the current item is flashed (by default for 1 sec)
you can define the flashing life time by yourself with the function

Example code :
ImGuiFileDialog::Instance()->SetFlashingAttenuationInSeconds(1.0f);

-----------------------------------------------------------------------------------------------------------------
## Bookmarks
-----------------------------------------------------------------------------------------------------------------

you can create/edit/call path bookmarks and load/save them in file

you can activate it by uncomment : "#define USE_BOOKMARK"

in you custom config file (CustomImGuiFileDialogConfig.h in this example)

you can also uncomment the next lines for customize it :
Example code :
#define bookmarkPaneWith 150.0f => width of the bookmark pane
#define IMGUI_TOGGLE_BUTTON ToggleButton => customize the Toggled button (button stamp must be : (const char* label, bool *toggle)
#define bookmarksButtonString "Bookmark" => the text in the toggle button
#define bookmarksButtonHelpString "Bookmark" => the helper text when mouse over the button
#define addBookmarkButtonString "+" => the button for add a bookmark
#define removeBookmarkButtonString "-" => the button for remove the selected bookmark


* you can select each bookmark for edit the displayed name corresponding to a path
* you must double click on the label for apply the bookmark

you can also serialize/deserialize bookmarks by ex for load/save from/to file : (check the app sample by ex)
Example code :
Load => ImGuiFileDialog::Instance()->DeserializeBookmarks(bookmarString);
Save => std::string bookmarkString = ImGuiFileDialog::Instance()->SerializeBookmarks();


-----------------------------------------------------------------------------------------------------------------
## Path Edition :
-----------------------------------------------------------------------------------------------------------------

if you click right on one of any path button, you can input or modify the path pointed by this button.
then press the validate key (Enter by default with GLFW) for validate the new path
or press the escape key (Escape by default with GLFW) for quit the input path edition

see in this gif doc/inputPathEdition.gif :
1) button edition with mouse button right and escape key for quit the edition
2) focus the input and press validation for set path

-----------------------------------------------------------------------------------------------------------------
## Confirm to OverWrite Dialog :
-----------------------------------------------------------------------------------------------------------------

If you want avoid OverWrite your files after confirmation,
you can show a Dialog for confirm or cancel the OverWrite operation.

You just need to define the flag ImGuiFileDialogFlags_ConfirmOverwrite
in your call to OpenDialog/OpenModal

By default this flag is not set, since there is no pre-defined way to
define if a dialog will be for Open or Save behavior. (and its wanted :) )

Example code For Standard Dialog :
Example code :
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
	ICON_IGFD_SAVE " Choose a File", filters,
	".", "", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite);

Example code For Modal Dialog :
Example code :
ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey",
	ICON_IGFD_SAVE " Choose a File", filters,
	".", "", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite);

This dialog will only verify the file in the file field.
So Not to be used with GetSelection()

The Confirm dialog will be a forced Modal Dialog, not moveable, displayed
in the center of the current FileDialog.

As usual you can customize the dialog,
in you custom config file (CustomImGuiFileDialogConfig.h in this example)

you can  uncomment the next lines for customize it :

Example code :
#define OverWriteDialogTitleString "The file Already Exist !"
#define OverWriteDialogMessageString "Would you like to OverWrite it ?"
#define OverWriteDialogConfirmButtonString "Confirm"
#define OverWriteDialogCancelButtonString "Cancel"

-----------------------------------------------------------------------------------------------------------------
## Open / Save dialog Behavior :
-----------------------------------------------------------------------------------------------------------------

There is no way to distinguish the "open dialog" behavior than "save dialog" behavior.
So you msut adapt the return according to your need :

if you want open file(s) or directory(s), you must use : GetSelection() method. you will obtain a std::map<FileName, FilePathName> of the selection
if you want create a file, you must use : GetFilePathName()/GetCurrentFileName()

the return method's and comments :

Example code :
std::map<std::string, std::string> GetSelection(); // Open File behavior : will return selection via a map<FileName, FilePathName>
std::string GetFilePathName();                     // Create File behavior : will always return the content of the field with current filter extention and current path
std::string GetCurrentFileName();                  // Create File behavior : will always return the content of the field with current filter extention
std::string GetCurrentPath();                      // will return current path
std::string GetCurrentFilter();                    // get selected filter
UserDatas GetUserDatas();                          // get user datas send with Open Dialog

-----------------------------------------------------------------------------------------------------------------
## C API
-----------------------------------------------------------------------------------------------------------------

A C API is available let you include ImGuiFileDialog in your C project.
btw, ImGuiFileDialog depend of ImGui and dirent (for windows)

Sample code with cimgui :

// create ImGuiFileDialog
ImGuiFileDialog *cfileDialog = IGFD_Create();

// open dialog
if (igButton("Open File", buttonSize))
{
	IGFD_OpenDialog(cfiledialog,
		"filedlg",                              // dialog key (make it possible to have different treatment reagrding the dialog key
		"Open a File",                          // dialog title
		"c files(*.c *.h){.c,.h}",              // dialog filter syntax : simple => .h,.c,.pp, etc and collections : text1{filter0,filter1,filter2}, text2{filter0,filter1,filter2}, etc..
		".",                                    // base directory for files scan
		"",                                     // base filename
		0,                                      // a fucntion for display a right pane if you want
		0.0f,                                   // base width of the pane
		0,                                      // count selection : 0 infinite, 1 one file (default), n (n files)
		"User data !",                          // some user datas
		ImGuiFileDialogFlags_ConfirmOverwrite); // ImGuiFileDialogFlags
}

ImGuiIO* ioptr = igGetIO();
ImVec2 maxSize;
maxSize.x = ioptr->DisplaySize.x * 0.8f;
maxSize.y = ioptr->DisplaySize.y * 0.8f;
ImVec2 minSize;
minSize.x = maxSize.x * 0.25f;
minSize.y = maxSize.y * 0.25f;

// display dialog
if (IGFD_DisplayDialog(cfiledialog, "filedlg", ImGuiWindowFlags_NoCollapse, minSize, maxSize))
{
	if (IGFD_IsOk(cfiledialog)) // result ok
	{
		char* cfilePathName = IGFD_GetFilePathName(cfiledialog);
		printf("GetFilePathName : %s\n", cfilePathName);
		char* cfilePath = IGFD_GetCurrentPath(cfiledialog);
		printf("GetCurrentPath : %s\n", cfilePath);
		char* cfilter = IGFD_GetCurrentFilter(cfiledialog);
		printf("GetCurrentFilter : %s\n", cfilter);
		// here convert from string because a string was passed as a userDatas, but it can be what you want
		void* cdatas = IGFD_GetUserDatas(cfiledialog);
		if (cdatas)
			printf("GetUserDatas : %s\n", (const char*)cdatas);
		struct IGFD_Selection csel = IGFD_GetSelection(cfiledialog); // multi selection
		printf("Selection :\n");
		for (int i = 0; i < (int)csel.count; i++)
		{
			printf("(%i) FileName %s => path %s\n", i, csel.table[i].fileName, csel.table[i].filePathName);
		}
		// action

		// destroy
		if (cfilePathName) free(cfilePathName);
		if (cfilePath) free(cfilePath);
		if (cfilter) free(cfilter);

		IGFD_Selection_DestroyContent(&csel);
	}
	IGFD_CloseDialog(cfiledialog);
}

// destroy ImGuiFileDialog
IGFD_Destroy(cfiledialog);

-----------------------------------------------------------------------------------------------------------------
## How to Integrate ImGuiFileDialog in your project
-----------------------------------------------------------------------------------------------------------------

### ImGuiFileDialog require :

* dirent v1.23 (https://github.com/tronkko/dirent/tree/v1.23) lib, only for windows. Successfully tested with version v1.23 only
* Dear ImGui (https://github.com/ocornut/imgui/tree/master) (with/without tables widgets)

### Customize ImGuiFileDialog :

You just need to write your own config file by override the file : ImGuiFileDialog/ImGuiFileDialogConfig.h
like i do here with CustomImGuiFileDialogConfig.h

After that, for let ImGuiFileDialog your own custom file,
you must define the preprocessor directive CUSTOM_IMGUIFILEDIALOG_CONFIG with the path of you custom config file.
This path must be relative to the directory where you put ImGuiFileDialog module.

-----------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------------

Thats all.

You can check by example in this repo with the file CustomImGuiFileDialogConfig.h :
- this trick was used for have custom icon font instead of labels for buttons or messages titles
- you can also use your custom imgui button, the button call stamp must be same by the way :)

The Custom Icon Font (in CustomFont.cpp and CustomFont.h) was made with ImGuiFontStudio (https://github.com/aiekick/ImGuiFontStudio) i wrote for that :)
ImGuiFontStudio is using also ImGuiFileDialog.

-----------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------------
*/

#ifndef IMGUIFILEDIALOG_H
#define IMGUIFILEDIALOG_H

#define IMGUIFILEDIALOG_VERSION "v0.5.5"

#ifndef CUSTOM_IMGUIFILEDIALOG_CONFIG
#include "ImGuiFileDialogConfig.h"
#else // CUSTOM_IMGUIFILEDIALOG_CONFIG
#include CUSTOM_IMGUIFILEDIALOG_CONFIG
#endif // CUSTOM_IMGUIFILEDIALOG_CONFIG

typedef int ImGuiFileDialogFlags; // -> enum ImGuiFileDialogFlags_
enum ImGuiFileDialogFlags_
{
	ImGuiFileDialogFlags_None = 0,
	ImGuiFileDialogFlags_ConfirmOverwrite = 1 << 0,
};

#ifdef __cplusplus

#include "imgui/imgui.h"

#include <cfloat>
#include <utility>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <list>

namespace IGFD
{
	#ifndef MAX_FILE_DIALOG_NAME_BUFFER 
	#define MAX_FILE_DIALOG_NAME_BUFFER 1024
	#endif // MAX_FILE_DIALOG_NAME_BUFFER

	#ifndef MAX_PATH_BUFFER_SIZE
	#define MAX_PATH_BUFFER_SIZE 1024
	#endif // MAX_PATH_BUFFER_SIZE

	struct FileExtentionInfosStruct
	{
		ImVec4 color = ImVec4(0, 0, 0, 0);
		std::string icon;
		FileExtentionInfosStruct() : color(0, 0, 0, 0) { }
		FileExtentionInfosStruct(const ImVec4& vColor, const std::string& vIcon = "") { color = vColor; icon = vIcon; }
	};

	typedef void* UserDatas;
	typedef std::function<void(const char*, UserDatas, bool*)> PaneFun;
	
	class FileDialog
	{

	///////////////////////////////////////////////////////////////////////////////////////
	/// PRIVATE STRUCTS / ENUMS ///////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	private:
#ifdef USE_BOOKMARK
		struct BookmarkStruct
		{
			std::string name;
			std::string path;
		};
#endif // USE_BOOKMARK

		enum class SortingFieldEnum
		{
			FIELD_NONE = 0,
			FIELD_FILENAME,
			FIELD_TYPE,
			FIELD_SIZE,
			FIELD_DATE
		};

		struct FileInfoStruct
		{
			char type = ' ';
			std::string filePath;
			std::string fileName;
			std::string fileName_optimized; // optimized for search => insensitivecase
			std::string ext;
			size_t fileSize = 0; // for sorting operations
			std::string formatedFileSize;
			std::string fileModifDate;
		};

		struct FilterInfosStruct
		{
			std::string filter;
			std::set<std::string> collectionfilters;
			void clear() {filter.clear(); collectionfilters.clear();}
			bool empty() const { return filter.empty() && collectionfilters.empty(); }
			bool filterExist(const std::string& vFilter) {return filter == vFilter || collectionfilters.find(vFilter) != collectionfilters.end(); }
		};

	///////////////////////////////////////////////////////////////////////////////////////
	/// PRIVATE VARIABLES /////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	private:
		std::vector<FileInfoStruct> m_FileList;
        std::vector<FileInfoStruct> m_FilteredFileList;
        std::unordered_map<std::string, FileExtentionInfosStruct> m_FileExtentionInfos;
		std::string m_CurrentPath;
		std::vector<std::string> m_CurrentPath_Decomposition;
		std::set<std::string> m_SelectedFileNames;
		std::string m_Name;
		bool m_ShowDialog = false;
		bool m_ShowDrives = false;
		std::string m_LastSelectedFileName; // for shift multi selectio
		std::vector<FilterInfosStruct> m_Filters;
		FilterInfosStruct m_SelectedFilter;
		bool m_InputPathActivated = false; // show input for path edition
        ImGuiListClipper m_FileListClipper;
		ImVec2 m_DialogCenterPos = ImVec2(0, 0); // center pos for display the confirm overwrite dialog
		int m_LastImGuiFrameCount = 0; // to be sure than only one dialog displayed per frame
		float m_FooterHeight = 0.0f;
		bool m_DrivesClicked = false; // events
		bool m_PathClicked = false;// events
		bool m_CanWeContinue = true;// events
		bool m_OkResultToConfirm = false; // to confim if ok for OverWrite
		bool m_IsOk = false;								
		bool m_CreateDirectoryMode = false;					// for create directory mode
		std::string m_HeaderFileName;						// detail view column file
		std::string m_HeaderFileType;						// detail view column type
		std::string m_HeaderFileSize;						// detail view column size
		std::string m_HeaderFileDate;						// detail view column date + time
		bool m_SortingDirection[4] = { true, true, true, true };	// detail view // true => Descending, false => Ascending
		SortingFieldEnum m_SortingField = SortingFieldEnum::FIELD_FILENAME;  // detail view sorting column

		std::string dlg_key;
		std::string dlg_title;
		std::string dlg_filters{};
		std::string dlg_path;
		std::string dlg_defaultFileName;
		std::string dlg_defaultExt;
		ImGuiFileDialogFlags dlg_flags = ImGuiFileDialogFlags_None;
		UserDatas dlg_userDatas = nullptr;
		PaneFun dlg_optionsPane = nullptr;
		float dlg_optionsPaneWidth = 0.0f;
		std::string searchTag;
		size_t dlg_countSelectionMax = 1; // 0 for infinite
		bool dlg_modal = false;

#ifdef USE_EXPLORATION_BY_KEYS
		size_t m_FlashedItem = 0;							// flash when select by char
		float m_FlashAlpha = 0.0f;							// flash when select by char
		float m_FlashAlphaAttenInSecs = 1.0f;				// fps display dependant
		size_t m_LocateFileByInputChar_lastFileIdx = 0;
		ImWchar m_LocateFileByInputChar_lastChar = 0;
		int m_LocateFileByInputChar_InputQueueCharactersSize = 0;
		bool m_LocateFileByInputChar_lastFound = false;
#endif // USE_EXPLORATION_BY_KEYS
#ifdef USE_BOOKMARK
		float m_BookmarkWidth = 200.0f;
        ImGuiListClipper m_BookmarkClipper;
		std::vector<BookmarkStruct> m_Bookmarks;
		bool m_BookmarkPaneShown = false;
#endif // USE_BOOKMARK

	///////////////////////////////////////////////////////////////////////////////////////
	/// PUBLIC PARAMS /////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	public:
		char InputPathBuffer[MAX_PATH_BUFFER_SIZE] = "";
		char FileNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";
		char DirectoryNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";
		char SearchBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";
		char VariadicBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";

#ifdef USE_BOOKMARK
		char BookmarkEditBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";
#endif // USE_BOOKMARK
		bool m_AnyWindowsHovered = false;					// not remember why haha :) todo : to check if we can remove

	///////////////////////////////////////////////////////////////////////////////////////
	/// PUBLIC METHODS/////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	public: 
		static FileDialog* Instance()								// Singleton for easier accces form anywhere but only one dialog at a time
		{
			static auto* _instance = new FileDialog();
			return _instance;
		}

	public:
		FileDialog();												// ImGuiFileDialog Constructor. can be used for have many dialog at same tiem (not possible with singleton)
		~FileDialog();												// ImGuiFileDialog Destructor

		// standard dialog
		void OpenDialog(											// open simple dialog (path and fileName can be specified)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vPath,								// path
			const std::string& vFileName,							// defaut file name
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		void OpenDialog(											// open simple dialog (path and filename are obtained from filePathName)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vFilePathName,						// file path name (will be decompsoed in path and fileName)
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		// with pane
		void OpenDialog(											// open dialog with custom right pane (path and fileName can be specified)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vPath,								// path
			const std::string& vFileName,							// defaut file name
			const PaneFun& vSidePane,								// side pane
			const float& vSidePaneWidth = 250.0f,					// side pane width
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		void OpenDialog(											// open dialog with custom right pane (path and filename are obtained from filePathName)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vFilePathName,						// file path name (will be decompsoed in path and fileName)
			const PaneFun& vSidePane,								// side pane
			const float& vSidePaneWidth = 250.0f,					// side pane width
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		// modal dialog
		void OpenModal(												// open simple modal (path and fileName can be specified)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vPath,								// path
			const std::string& vFileName,							// defaut file name
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		void OpenModal(												// open simple modal (path and fielname are obtained from filePathName)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vFilePathName,						// file path name (will be decompsoed in path and fileName)
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		// with pane
		void OpenModal(												// open modal with custom right pane (path and filename are obtained from filePathName)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vPath,								// path
			const std::string& vFileName,							// defaut file name
			const PaneFun& vSidePane,								// side pane
			const float& vSidePaneWidth = 250.0f,					// side pane width
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		void OpenModal(												// open modal with custom right pane (path and fielname are obtained from filePathName)
			const std::string& vKey,								// key dialog
			const std::string& vTitle,								// title
			const char* vFilters,									// filters
			const std::string& vFilePathName,						// file path name (will be decompsoed in path and fileName)
			const PaneFun& vSidePane,								// side pane
			const float& vSidePaneWidth = 250.0f,					// side pane width
			const int& vCountSelectionMax = 1,						// count selection max
			UserDatas vUserDatas = nullptr,							// user datas (can be retrieved in pane)
			ImGuiFileDialogFlags vFlags = 0);						// ImGuiFileDialogFlags 

		// Display / Close dialog form
		bool Display(												// Display the dialog. return true if a result was obtained (Ok or not)
			const std::string& vKey,								// key dialog to display (if not the same key as defined by OpenDialog/Modal => no opening)
			ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse,	// ImGuiWindowFlags
			ImVec2 vMinSize = ImVec2(0, 0),							// mininmal size contraint for the ImGuiWindow
			ImVec2 vMaxSize = ImVec2(FLT_MAX, FLT_MAX));			// maximal size contraint for the ImGuiWindow
		void Close();												// close dialog

		// queries
		bool WasOpenedThisFrame(const std::string& vKey);			// say if the dialog key was already opened this frame
		bool WasOpenedThisFrame();									// say if the dialog was already opened this frame
		bool IsOpened(const std::string& vKey);						// say if the key is opened
		bool IsOpened();											// say if the dialog is opened somewhere	
		std::string GetOpenedKey();									// return the dialog key who is opened, return nothing if not opened

		// get result
		bool IsOk();												// true => Dialog Closed with Ok result / false : Dialog closed with cancel result
		std::map<std::string, std::string> GetSelection();			// Open File behavior : will return selection via a map<FileName, FilePathName>
		std::string GetFilePathName();								// Save File behavior : will always return the content of the field with current filter extention and current path
		std::string GetCurrentFileName();							// Save File behavior : will always return the content of the field with current filter extention
		std::string GetCurrentPath();								// will return current path
		std::string GetCurrentFilter();								// will return selected filter
		UserDatas GetUserDatas();									// will return user datas send with Open Dialog/Modal
		
		// extentions displaying
		void SetExtentionInfos(										// SetExtention datas for have custom display of particular file type
			const std::string& vFilter,								// extention filter to tune
			const FileExtentionInfosStruct& vInfos);				// Filter Extention Struct who contain Color and Icon/Text for the display of the file with extention filter
		void SetExtentionInfos(										// SetExtention datas for have custom display of particular file type
			const std::string& vFilter,								// extention filter to tune
			const ImVec4& vColor,									// wanted color for the display of the file with extention filter
			const std::string& vIcon = "");							// wanted text or icon of the file with extention filter
		bool GetExtentionInfos(										// GetExtention datas. return true is extention exist
			const std::string& vFilter,								// extention filter (same as used in SetExtentionInfos)
			ImVec4 *vOutColor,										// color to retrieve
			std::string* vOutIcon = 0);								// icon or text to retrieve
		void ClearExtentionInfos();									// clear extentions setttings

#ifdef USE_EXPLORATION_BY_KEYS
		void SetFlashingAttenuationInSeconds(						// set the flashing time of the line in file list when use exploration keys
			float vAttenValue);										// set the attenuation (from flashed to not flashed) in seconds
#endif // USE_EXPLORATION_BY_KEYS
#ifdef USE_BOOKMARK
		std::string SerializeBookmarks();							// serialize bookmarks : return bookmark buffer to save in a file
		void DeserializeBookmarks(									// deserialize bookmarks : load bookmar buffer to load in the dialog (saved from previous use with SerializeBookmarks())
			const std::string& vBookmarks);							// bookmark buffer to load
#endif // USE_BOOKMARK

	///////////////////////////////////////////////////////////////////////////////////////
	/// PROTECTED'S METHODS ///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////

	protected: 
		// dialog parts
		virtual void DrawHeader();									// draw header part of the dialog (bookmark btn, dir creation, path composer, search bar)
		virtual void DrawContent();									// draw content part of the dialog (bookmark pane, file list, side pane)
		virtual bool DrawFooter();									// draw footer part of the dialog (file field, fitler combobox, ok/cancel btn's)

		// widgets components
		virtual void DrawDirectoryCreation();						// draw directory creation widget
		virtual void DrawPathComposer();							// draw path composer widget
		virtual void DrawSearchBar();								// draw search bar
		virtual void DrawFileListView(ImVec2 vSize);				// draw file list viexw
		virtual void DrawSidePane(float vHeight);					// draw side pane
#ifdef USE_BOOKMARK
		virtual void DrawBookMark();								// draw bookmark button
#endif // USE_BOOKMARK

		// others
		bool SelectableItem(int vidx, const FileInfoStruct& vInfos, bool vSelected, const char* vFmt, ...);					// selectable item for table
		void ResetEvents();																									// reset events (path, drives, continue)
		void SetDefaultFileName(const std::string& vFileName);																// set default file name
		bool SelectDirectory(const FileInfoStruct& vInfos);																	// enter directory 
		void SelectFileName(const FileInfoStruct& vInfos);																	// select filename
		void RemoveFileNameInSelection(const std::string& vFileName);														// selection : remove a file name
		void AddFileNameInSelection(const std::string& vFileName, bool vSetLastSelectionFileName);							// selection : add a file name
		void SetPath(const std::string& vPath);																				// set the path of the dialog, will launch the directory scan for populate the file listview
		void CompleteFileInfos(FileInfoStruct *vFileInfoStruct);															// set time and date infos of a file (detail view mode)
		void SortFields(SortingFieldEnum vSortingField = SortingFieldEnum::FIELD_NONE, 	bool vCanChangeOrder = false);		// will sort a column
		void ScanDir(const std::string& vPath);																				// scan the directory for retrieve the file list
		void SetCurrentDir(const std::string& vPath);																		// define current directory for scan
		bool CreateDir(const std::string& vPath);																			// create a directory on the file system
		std::string ComposeNewPath(std::vector<std::string>::iterator vIter);												// compose a path from the compose path widget
		void GetDrives();																									// list drives on windows platform
		void ParseFilters(const char* vFilters);																			// parse filter syntax, detect and parse filter collection
		void SetSelectedFilterWithExt(const std::string& vFilter);															// select filter
		static std::string OptimizeFilenameForSearchOperations(std::string vFileName);										// easier the search by lower case all filenames
	    void ApplyFilteringOnFileList();																					// filter the file list accroding to the searh tags
		bool Confirm_Or_OpenOverWriteFileDialog_IfNeeded(bool vLastAction, ImGuiWindowFlags vFlags);						// treatment of the result, start the confirm to overwrite dialog if needed (if defined with flag)
		bool IsFileExist(const std::string& vFile);																			// is file exist

#ifdef USE_EXPLORATION_BY_KEYS
		// file localization by input chat // widget flashing
		void LocateByInputKey();																							// select a file line in listview according to char key
		bool LocateItem_Loop(ImWchar vC);																					// restrat for start of list view if not found a corresponding file
		void ExploreWithkeys();																								// select file/directory line in listview accroding to up/down enter/backspace keys
		static bool FlashableSelectable(																					// custom flashing selectable widgets, for flash the selected line in a short time
			const char* label, bool selected = false, ImGuiSelectableFlags flags = 0,
			bool vFlashing = false, const ImVec2& size = ImVec2(0, 0));
		void StartFlashItem(size_t vIdx);																					// define than an item must be flashed
		bool BeginFlashItem(size_t vIdx);																					// start the flashing of a line in lsit view
		void EndFlashItem();																								// end the fleshing accrdoin to var m_FlashAlphaAttenInSecs
#endif // USE_EXPLORATION_BY_KEYS

#ifdef USE_BOOKMARK
		void DrawBookmarkPane(ImVec2 vSize);																				// draw bookmark pane
#endif // USE_BOOKMARK
	};
}
typedef IGFD::UserDatas IGFDUserDatas;
typedef IGFD::PaneFun IGFDPaneFun;
typedef IGFD::FileDialog ImGuiFileDialog;
#else // __cplusplus
	typedef struct ImGuiFileDialog ImGuiFileDialog;
	typedef struct IGFD_Selection_Pair IGFD_Selection_Pair;
	typedef struct IGFD_Selection IGFD_Selection;
#endif // __cplusplus

// C Interface

#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
	#ifdef IMGUIFILEDIALOG_NO_EXPORT
		#define API
	#else // IMGUIFILEDIALOG_NO_EXPORT
		#define API __declspec(dllexport)
	#endif // IMGUIFILEDIALOG_NO_EXPORT
#else // defined _WIN32 || defined __CYGWIN__
	#ifdef __GNUC__
		#define API  __attribute__((__visibility__("default")))
	#else // __GNUC__
		#define API
	#endif // __GNUC__
#endif // defined _WIN32 || defined __CYGWIN__

#ifdef __cplusplus
	#define IMGUIFILEDIALOG_API extern "C" API 
#else // __cplusplus
	#define IMGUIFILEDIALOG_API
#endif // __cplusplus

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// C API ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct IGFD_Selection_Pair
	{
		char* fileName;
		char* filePathName;
	};

	IMGUIFILEDIALOG_API IGFD_Selection_Pair IGFD_Selection_Pair_Get();										// return an initialized IGFD_Selection_Pair			
	IMGUIFILEDIALOG_API void IGFD_Selection_Pair_DestroyContent(IGFD_Selection_Pair *vSelection_Pair);		// destroy the content of a IGFD_Selection_Pair
	
	struct IGFD_Selection
	{
		IGFD_Selection_Pair* table;	// 0
		size_t count;						// 0U
	};

	IMGUIFILEDIALOG_API IGFD_Selection IGFD_Selection_Get();												// return an initialized IGFD_Selection
	IMGUIFILEDIALOG_API void IGFD_Selection_DestroyContent(IGFD_Selection* vSelection);						// destroy the content of a IGFD_Selection

	// constructor / destructor
	IMGUIFILEDIALOG_API ImGuiFileDialog* IGFD_Create(void);													// create the filedialog context
	IMGUIFILEDIALOG_API void IGFD_Destroy(ImGuiFileDialog *vContext);										// destroy the filedialog context

	typedef void (*IGFD_PaneFun)(const char*, void*, bool*);												// callback fucntion for display the pane
	
	IMGUIFILEDIALOG_API void IGFD_OpenDialog(					// open a standard dialog
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vPath,										// path
		const char* vFileName,									// defaut file name
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 
	
	IMGUIFILEDIALOG_API void IGFD_OpenDialog2(					// open a standard dialog
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vFilePathName,								// defaut file path name (path and filename witl be extracted from it)
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API void IGFD_OpenPaneDialog(				// open a standard dialog with pane
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vPath,										// path
		const char* vFileName,									// defaut file name
		const IGFD_PaneFun vSidePane,							// side pane
		const float vSidePaneWidth,								// side pane base width
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API void IGFD_OpenPaneDialog2(				// open a standard dialog with pane
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vFilePathName,								// defaut file name (path and filename witl be extracted from it)
		const IGFD_PaneFun vSidePane,							// side pane
		const float vSidePaneWidth,								// side pane base width
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API void IGFD_OpenModal(					// open a modal dialog
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vPath,										// path
		const char* vFileName,									// defaut file name
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API void IGFD_OpenModal2(					// open a modal dialog
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vFilePathName,								// defaut file name (path and filename witl be extracted from it)
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API void IGFD_OpenPaneModal(				// open a modal dialog with pane
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vPath,										// path
		const char* vFileName,									// defaut file name
		const IGFD_PaneFun vSidePane,							// side pane
		const float vSidePaneWidth,								// side pane base width
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API void IGFD_OpenPaneModal2(				// open a modal dialog with pane
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog
		const char* vTitle,										// title
		const char* vFilters,									// filters/filter collections. set it to null for directory mode 
		const char* vFilePathName,								// defaut file name (path and filename witl be extracted from it)
		const IGFD_PaneFun vSidePane,							// side pane
		const float vSidePaneWidth,								// side pane base width
		const int vCountSelectionMax,							// count selection max
		void* vUserDatas,										// user datas (can be retrieved in pane)
		ImGuiFileDialogFlags vFlags);							// ImGuiFileDialogFlags 

	IMGUIFILEDIALOG_API bool IGFD_DisplayDialog(				// Display the dialog
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context
		const char* vKey,										// key dialog to display (if not the same key as defined by OpenDialog/Modal => no opening)
		ImGuiWindowFlags vFlags,								// ImGuiWindowFlags
		ImVec2 vMinSize,										// mininmal size contraint for the ImGuiWindow
		ImVec2 vMaxSize);										// maximal size contraint for the ImGuiWindow

	IMGUIFILEDIALOG_API void IGFD_CloseDialog(					// Close the dialog
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context			

	IMGUIFILEDIALOG_API bool IGFD_IsOk(							// true => Dialog Closed with Ok result / false : Dialog closed with cancel result
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context		

	IMGUIFILEDIALOG_API bool IGFD_WasKeyOpenedThisFrame(		// say if the dialog key was already opened this frame
		ImGuiFileDialog* vContext, 								// ImGuiFileDialog context		
		const char* vKey);

	IMGUIFILEDIALOG_API bool IGFD_WasOpenedThisFrame(			// say if the dialog was already opened this frame
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context	

	IMGUIFILEDIALOG_API bool IGFD_IsKeyOpened(					// say if the dialog key is opened
		ImGuiFileDialog* vContext, 								// ImGuiFileDialog context		
		const char* vCurrentOpenedKey);							// the dialog key

	IMGUIFILEDIALOG_API bool IGFD_IsOpened(						// say if the dialog is opened somewhere	
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context		
	
	IMGUIFILEDIALOG_API IGFD_Selection IGFD_GetSelection(		// Open File behavior : will return selection via a map<FileName, FilePathName>
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context		

	IMGUIFILEDIALOG_API char* IGFD_GetFilePathName(				// Save File behavior : will always return the content of the field with current filter extention and current path
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context				

	IMGUIFILEDIALOG_API char* IGFD_GetCurrentFileName(			// Save File behavior : will always return the content of the field with current filter extention
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context				

	IMGUIFILEDIALOG_API char* IGFD_GetCurrentPath(				// will return current path
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context					

	IMGUIFILEDIALOG_API char* IGFD_GetCurrentFilter(			// will return selected filter
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context						

	IMGUIFILEDIALOG_API void* IGFD_GetUserDatas(				// will return user datas send with Open Dialog/Modal
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context											

	IMGUIFILEDIALOG_API void IGFD_SetExtentionInfos(			// SetExtention datas for have custom display of particular file type
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context 
		const char* vFilter,									// extention filter to tune
		ImVec4 vColor,											// wanted color for the display of the file with extention filter
		const char* vIconText);									// wanted text or icon of the file with extention filter (can be sued with font icon)

	IMGUIFILEDIALOG_API void IGFD_SetExtentionInfos2(			// SetExtention datas for have custom display of particular file type
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context 
		const char* vFilter,									// extention filter to tune
		float vR, float vG, float vB, float vA,					// wanted color channels RGBA for the display of the file with extention filter
		const char* vIconText);									// wanted text or icon of the file with extention filter (can be sued with font icon)

	IMGUIFILEDIALOG_API bool IGFD_GetExtentionInfos(
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context 
		const char* vFilter,									// extention filter (same as used in SetExtentionInfos)
		ImVec4* vOutColor,										// color to retrieve
		char** vOutIconText);									// icon or text to retrieve

	IMGUIFILEDIALOG_API void IGFD_ClearExtentionInfos(			// clear extentions setttings
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context

#ifdef USE_EXPLORATION_BY_KEYS
	IMGUIFILEDIALOG_API void IGFD_SetFlashingAttenuationInSeconds(	// set the flashing time of the line in file list when use exploration keys
		ImGuiFileDialog* vContext,									// ImGuiFileDialog context 
		float vAttenValue);											// set the attenuation (from flashed to not flashed) in seconds
#endif

#ifdef USE_BOOKMARK
	IMGUIFILEDIALOG_API char* IGFD_SerializeBookmarks(			// serialize bookmarks : return bookmark buffer to save in a file
		ImGuiFileDialog* vContext);								// ImGuiFileDialog context

	IMGUIFILEDIALOG_API void IGFD_DeserializeBookmarks(			// deserialize bookmarks : load bookmar buffer to load in the dialog (saved from previous use with SerializeBookmarks())
		ImGuiFileDialog* vContext,								// ImGuiFileDialog context 
		const char* vBookmarks);								// bookmark buffer to load 
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // IMGUIFILEDIALOG_H