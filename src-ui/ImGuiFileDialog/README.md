[<img src="https://github.com/aiekick/ImGuiFileDialog/workflows/Win/badge.svg" width="150"/>](https://github.com/aiekick/ImGuiFileDialog/actions?query=workflow%3AWin) 
[<img src="https://github.com/aiekick/ImGuiFileDialog/workflows/Linux/badge.svg" width="165"/>](https://github.com/aiekick/ImGuiFileDialog/actions?query=workflow%3ALinux)
[<img src="https://github.com/aiekick/ImGuiFileDialog/workflows/Osx/badge.svg" width="150"/>](https://github.com/aiekick/ImGuiFileDialog/actions?query=workflow%3AOsx)

# ImGuiFileDialog

## Purpose

ImGuiFileDialog is a file selection dialog built for (and using only) [Dear ImGui](https://github.com/ocornut/imgui).

My primary goal was to have a custom pane with widgets according to file extension. This was not possible using other solutions.

## Structure of this repo

* The library is in [Lib_Only branch](https://github.com/aiekick/ImGuiFileDialog/tree/Lib_Only)
* A demo app can be found the [master branch](https://github.com/aiekick/ImGuiFileDialog/tree/master)

This library is designed to be dropped into your source code rather than compiled separately.

From your project directory:

```
mkdir lib
cd lib
git clone https://github.com/aiekick/ImGuiFileDialog.git
git checkout Lib_Only
```

These commands create a `lib` directory where you can store any third-party dependencies used in your 
project, downloads the ImGuiFileDialog git repository and checks out the Lib_Only branch where the 
actual library code is located.

Add `lib/ImGuiFileDialog/ImGuiFileDialog.cpp` to your build system and include 
`lib/ImGuiFileDialog/ImGuiFileDialog.h` in your source code. ImGuiFileLib will compile with and be 
included directly in your executable file.

If, for example, your project uses cmake, look for a line like `add_executable(my_project_name main.cpp)` 
and change it to `add_executable(my_project_name lib/ImGuiFileDialog/ImGuiFileDialog.cpp main.cpp)`. 
This tells the compiler where to find the source code declared in `ImGuiFileDialog.h` which you included in your own source code.

You must also, of course, have added [Dear ImGui](https://github.com/ocornut/imgui) to your project for this to work at all.


## Features

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
- Support dir/files Exploring with keys : Up / Down / Enter (open dir) / Backspace (come back)
- Support dir/files Exploring by input char (case insensitive)
- Support bookmark creation/edition/call for directory (can have custom name corresponding to a path)
- Support input path edition by right click on a path button
- Support of a 'Confirm to Overwrite" dialog if File Exist
- A C Api is available (Succesfully testec with CimGui

## Singleton Pattern vs. Multiple Instances

### Single Dialog : 

If you only need to display one file dialog at a time, use ImGuiFileDialog's singleton pattern to avoid explicitly declaring an object:

```cpp
ImGuiFileDialog::Instance()->method_of_your_choice();
```

### Multiple Dialogs :

If you need to have multiple file dialogs open at once, declare each dialog explicity:

```cpp
ImGuiFileDialog instance_a;
instance_a.method_of_your_choice();
ImGuiFileDialog instance_b;
instance_b.method_of_your_choice();
```

## Simple Dialog :
```cpp
void drawGui()
{ 
  // open Dialog Simple
  if (ImGui::Button("Open File Dialog"))
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".cpp,.h,.hpp", ".");

  // display
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
  {
    // action if OK
    if (ImGuiFileDialog::Instance()->IsOk())
    {
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      // action
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }
}
```
![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/dlg_simple.gif)


## Directory Chooser :

To have a directory chooser, set the file extension filter to nullptr:
```cpp
ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose a Directory", nullptr, ".");
```

In this mode you can select any directory with one click and open a directory with a double-click.

![directoryChooser](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/directoryChooser.gif)


## Dialog with Custom Pane :

The signature of the custom pane callback is:

### for C++ :
```cpp
void(const char *vFilter, IGFDUserDatas vUserDatas, bool *vCantContinue)
```

### for C :
```c
void(const char *vFilter, void* vUserDatas, bool *vCantContinue)
```

### Example :

```cpp
static bool canValidateDialog = false;
inline void InfosPane(cosnt char *vFilter, IGFDUserDatas vUserDatas, bool *vCantContinue) // if vCantContinue is false, the user cant validate the dialog
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
            ".", "", std::bind(&InfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 350, 1, UserDatas("InfosPane"));

  // display and action if ok
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) 
  {
    if (ImGuiFileDialog::Instance()->IsOk())
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
    ImGuiFileDialog::Instance()->Close();
  }
}
```
![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/doc/dlg_with_pane.gif)

## Filter Infos

You can define a color for a filter type
```cpp
ImGuiFileDialog::Instance()->SetExtentionInfos(".cpp", ImVec4(1,1,0, 0.9));
ImGuiFileDialog::Instance()->SetExtentionInfos(".h", ImVec4(0,1,0, 0.9));
ImGuiFileDialog::Instance()->SetExtentionInfos(".hpp", ImVec4(0,0,1, 0.9));
ImGuiFileDialog::Instance()->SetExtentionInfos(".md", ImVec4(1,0,1, 0.9));
```

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/color_filter.png)

ImGuiFileDialog accepts icon font macros as well as text tags for file types. See [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) for a widely used Dear Imgui icon font helper or with [ImGuIFontStudio](https://github.com/aiekick/ImGuiFontStudio) (a gui tool for easily doing the same as IconFontCppHeaders)

```cpp
// add an icon for png files 
ImGuiFileDialog::Instance()->SetExtentionInfos(".png", ImVec4(0,1,1,0.9), ICON_IMFDLG_FILE_TYPE_PIC);
// add a text for gif files (the default value is [File] 
ImGuiFileDialog::Instance()->SetExtentionInfos(".gif", ImVec4(0, 1, 0.5, 0.9), "[GIF]");
```

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/filter_Icon.png)

## Filter Collections 

You can define a custom filter name that corresponds to a group of filters using this syntax:

```custom_name1{filter1,filter2,filter3},custom_name2{filter1,filter2},filter1```

When you select custom_name1, filters 1 to 3 will be applied. The characters `{` and `}` are reserved. Don't use them for filter names.

this code :
```cpp
const char *filters = "Source files (*.cpp *.h *.hpp){.cpp,.h,.hpp},Image files (*.png *.gif *.jpg *.jpeg){.png,.gif,.jpg,.jpeg},.md";
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", ICON_IMFDLG_FOLDER_OPEN " Choose a File", filters, ".");
```
will produce :
![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/collectionFilters.gif)

## Multi Selection

You can define in OpenDialog/OpenModal call the count file you want to select :
- 0 => infinite
- 1 => one file only (default)
- n => n files only

See the define at the end of these funcs after path.

```cpp
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".*,.cpp,.h,.hpp", ".");
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose 1 File", ".*,.cpp,.h,.hpp", ".", 1);
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose 5 File", ".*,.cpp,.h,.hpp", ".", 5);
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose many File", ".*,.cpp,.h,.hpp", ".", 0);
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".png,.jpg",
   ".", "", std::bind(&InfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 350, 1, "SaveFile"); // 1 file
```

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/multiSelection.gif)

## File Dialog Constraints

You can set the minimum and/or maximum size of the dialog:

```cpp
ImVec2 maxSize = ImVec2((float)display_w, (float)display_h);  // The full display area
ImVec2 minSize = maxSize * 0.5f;  // Half the display area
ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, minSize, maxSize);
```

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/dialog_constraints.gif)

## Detail View Mode

You can have tables display like that.

- uncomment "#define USE_IMGUI_TABLES" in you custom config file (CustomImGuiFileDialogConfig.h in this example)
- will be used by default when ImGui Table will be released in master
![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/imgui_tables_branch.gif)

## Exploring by keys

you can activate this feature by uncomment : "#define USE_EXPLORATION_BY_KEYS" 
in you custom config file (CustomImGuiFileDialogConfig.h in this example)

you can also uncomment the next lines for define your keys :

* IGFD_KEY_UP => Up key for explore to the top
* IGFD_KEY_DOWN => Down key for explore to the bottom
* IGFD_KEY_ENTER => Enter key for open directory
* IGFD_KEY_BACKSPACE => BackSpace for comming back to the last directory

you can also explore a file list by use the current key char.

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/explore_ny_keys.gif)

as you see the current item is flashed (by default for 1 sec)
you can define the flashing life time by yourself with the function
```cpp
ImGuiFileDialog::Instance()->SetFlashingAttenuationInSeconds(1.0f);
```
## Bookmarks

you can create/edit/call path bookmarks and load/save them in file

you can activate it by uncomment : "#define USE_BOOKMARK"

in you custom config file (CustomImGuiFileDialogConfig.h in this example)

you can also uncomment the next lines for customize it :
```cpp
#define bookmarkPaneWith 150.0f => width of the bookmark pane
#define IMGUI_TOGGLE_BUTTON ToggleButton => customize the Toggled button (button stamp must be : (const char* label, bool *toggle)
#define bookmarksButtonString "Bookmark" => the text in the toggle button
#define bookmarksButtonHelpString "Bookmark" => the helper text when mouse over the button
#define addBookmarkButtonString "+" => the button for add a bookmark
#define removeBookmarkButtonString "-" => the button for remove the selected bookmark
```

* you can select each bookmark for edit the displayed name corresponding to a path
* you must double click on the label for apply the bookmark 

![bookmarks.gif](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/bookmarks.gif)

you can also serialize/deserialize bookmarks by ex for load/save from/to file : (check the app sample by ex)
```cpp
Load => ImGuiFileDialog::Instance()->DeserializeBookmarks(bookmarString);
Save => std::string bookmarkString = ImGuiFileDialog::Instance()->SerializeBookmarks();
```

## Path Edition :

Right clicking on any path element button allows the user to manually edit the path from that portion of the tree.
Pressing the completion key (GLFW uses `enter` by default) validates the new path.
Pressing the cancel key (GLFW uses`escape` by default) cancels the manual entry and restores the original path.

Here's the manual entry operation in action:
![inputPathEdition.gif](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/inputPathEdition.gif)

## Confirm Overwrite Dialog :

If you want avoid overwriting files after selection, ImGuiFileDialog can show a dialog to confirm or cancel the operation.

To do so, define the flag ImGuiFileDialogFlags_ConfirmOverwrite in your call to OpenDialog/OpenModal.

By default this flag is not set since there is no pre-defined way to 
define if a dialog will be for Open or Save behavior. (by design! :) )

Example code For Standard Dialog :
```cpp
ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
    ICON_IGFD_SAVE " Choose a File", filters,
    ".", "", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite);
```

Example code For Modal Dialog :
```cpp
ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey",
    ICON_IGFD_SAVE " Choose a File", filters,
    ".", "", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite);
```

This dialog will only verify the file in the file field, not with `GetSelection()`.
 
The confirmation dialog will be a non-movable modal (input blocking) dialog displayed
in the middle of the current ImGuiFileDialog window.

As usual, you can customize the dialog in your custom config file (CustomImGuiFileDialogConfig.h in this example)

Uncomment these line for customization options:
```cpp
//#define OverWriteDialogTitleString "The file Already Exist !"
//#define OverWriteDialogMessageString "Would you like to OverWrite it ?"
//#define OverWriteDialogConfirmButtonString "Confirm"
//#define OverWriteDialogCancelButtonString "Cancel"
```

See the result :

![ConfirmToOverWrite.gif](https://github.com/aiekick/ImGuiFileDialog/blob/master/doc/ConfirmToOverWrite.gif)

## Open / Save dialog Behavior :

ImGuiFileDialog uses the same code internally for Open and Save dialogs. To distinguish between them access the 
various data return functions depending on what the dialog is doing.

When selecting an existing file (for example, a Load or Open dialog), use

```cpp
std::map<std::string, std::string> GetSelection(); // Returns selection via a map<FileName, FilePathName>
UserDatas GetUserDatas();                          // Get user data provided by the Open dialog
```

To selecting a new file (for example, a Save As... dialog), use:

```cpp
std::string GetFilePathName();                     // Returns the content of the selection field with current file extension and current path
std::string GetCurrentFileName();                  // Returns the content of the selection field with current file extension but no path
std::string GetCurrentPath();                      // Returns current path only
std::string GetCurrentFilter();                    // The file extension
```


## How to Integrate ImGuiFileDialog in your project

### ImGuiFileDialog require :

* [ImGui](https://github.com/ocornut/imgui/blob/master) (with/without tables widgets)
* [dirent v1.23](https://github.com/tronkko/dirent/tree/v1.23) lib, only for windows. already placed in the directory

### Customize ImGuiFileDialog :

You can customize many aspects of ImGuiFileDialog by overriding `ImGuiFileDialogConfig.h`. 

To enable your customizations, define the preprocessor directive CUSTOM_IMGUIFILEDIALOG_CONFIG with the path of your custom config file.
This path must be relative to the directory where you put the ImGuiFileDialog module.

This operation is demonstrated in `CustomImGuiFileDialog.h` in the example project to:
* Have a custom icon font instead of labels for buttons or message titles
* Customize the button text (the button call signature must be the same, by the way! :)

The Custom Icon Font (in [CustomFont.cpp](CustomFont.cpp) and [CustomFont.h](CustomFont.h)) was made with [ImGuiFontStudio](https://github.com/aiekick/ImGuiFontStudio) i wrote for that :)
ImGuiFontStudio is using also ImGuiFileDialog.

## Api's C/C++ :

### C++.

this is the base API :

```cpp
static FileDialog* Instance()                      // Singleton for easier accces form anywhere but only one dialog at a time

FileDialog();                                      // ImGuiFileDialog Constructor. can be used for have many dialog at same tiem (not possible with singleton)

// standard dialog
void OpenDialog(                                   // open simple dialog (path and fileName can be specified)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vPath,                      // path
    const std::string& vFileName,                  // defaut file name
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

void OpenDialog(                                   // open simple dialog (path and filename are obtained from filePathName)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vFilePathName,              // file path name (will be decompsoed in path and fileName)
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

// with pane
void OpenDialog(                                   // open dialog with custom right pane (path and fileName can be specified)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vPath,                      // path
    const std::string& vFileName,                  // defaut file name
    const PaneFun& vSidePane,                      // side pane
    const float& vSidePaneWidth = 250.0f,          // side pane width
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

void OpenDialog(                                   // open dialog with custom right pane (path and filename are obtained from filePathName)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vFilePathName,              // file path name (will be decompsoed in path and fileName)
    const PaneFun& vSidePane,                      // side pane
    const float& vSidePaneWidth = 250.0f,          // side pane width
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

// modal dialog
void OpenModal(                                    // open simple modal (path and fileName can be specified)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vPath,                      // path
    const std::string& vFileName,                  // defaut file name
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

void OpenModal(                                    // open simple modal (path and fielname are obtained from filePathName)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vFilePathName,              // file path name (will be decompsoed in path and fileName)
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

// with pane
void OpenModal(                                    // open modal with custom right pane (path and filename are obtained from filePathName)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vPath,                      // path
    const std::string& vFileName,                  // defaut file name
    const PaneFun& vSidePane,                      // side pane
    const float& vSidePaneWidth = 250.0f,                               // side pane width
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

void OpenModal(                                    // open modal with custom right pane (path and fielname are obtained from filePathName)
    const std::string& vKey,                       // key dialog
    const std::string& vTitle,                     // title
    const char* vFilters,                          // filters
    const std::string& vFilePathName,              // file path name (will be decompsoed in path and fileName)
    const PaneFun& vSidePane,                      // side pane
    const float& vSidePaneWidth = 250.0f,                               // side pane width
    const int& vCountSelectionMax = 1,             // count selection max
    UserDatas vUserDatas = nullptr,                // user datas (can be retrieved in pane)
    ImGuiFileDialogFlags vFlags = 0);              // ImGuiFileDialogFlags 

// Display / Close dialog form
bool Display(                                      // Display the dialog. return true if a result was obtained (Ok or not)
    const std::string& vKey,                       // key dialog to display (if not the same key as defined by OpenDialog/Modal => no opening)
    ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse, // ImGuiWindowFlags
    ImVec2 vMinSize = ImVec2(0, 0),                // mininmal size contraint for the ImGuiWindow
    ImVec2 vMaxSize = ImVec2(FLT_MAX, FLT_MAX));   // maximal size contraint for the ImGuiWindow
void Close();                                      // close dialog

// queries
bool WasOpenedThisFrame(const std::string& vKey);  // say if the dialog key was already opened this frame
bool WasOpenedThisFrame();                         // say if the dialog was already opened this frame
bool IsOpened(const std::string& vKey);            // say if the key is opened
bool IsOpened();                                   // say if the dialog is opened somewhere    
std::string GetOpenedKey();                        // return the dialog key who is opened, return nothing if not opened

// get result
bool IsOk();                                       // true => Dialog Closed with Ok result / false : Dialog closed with cancel result
std::map<std::string, std::string> GetSelection(); // Open File behavior : will return selection via a map<FileName, FilePathName>
std::string GetFilePathName();                     // Save File behavior : will always return the content of the field with current filter extention and current path
std::string GetCurrentFileName();                  // Save File behavior : will always return the content of the field with current filter extention
std::string GetCurrentPath();                      // will return current path
std::string GetCurrentFilter();                    // will return selected filter
UserDatas GetUserDatas();                          // will return user datas send with Open Dialog/Modal
        
           // extentions displaying
void SetExtentionInfos(                            // SetExtention datas for have custom display of particular file type
    const std::string& vFilter,                    // extention filter to tune
    const FileExtentionInfosStruct& vInfos);       // Filter Extention Struct who contain Color and Icon/Text for the display of the file with extention filter
void SetExtentionInfos(                            // SetExtention datas for have custom display of particular file type
    const std::string& vFilter,                    // extention filter to tune
    const ImVec4& vColor,                          // wanted color for the display of the file with extention filter
    const std::string& vIcon = "");                // wanted text or icon of the file with extention filter
bool GetExtentionInfos(                            // GetExtention datas. return true is extention exist
    const std::string& vFilter,                    // extention filter (same as used in SetExtentionInfos)
    ImVec4 *vOutColor,                             // color to retrieve
    std::string* vOutIcon = 0);                    // icon or text to retrieve
void ClearExtentionInfos();                        // clear extentions setttings

feature : USE_EXPLORATION_BY_KEYS
void SetFlashingAttenuationInSeconds(              // set the flashing time of the line in file list when use exploration keys
    float vAttenValue);                            // set the attenuation (from flashed to not flashed) in seconds

feature : USE_BOOKMARK
std::string SerializeBookmarks();                  // serialize bookmarks : return bookmark buffer to save in a file
void DeserializeBookmarks(                         // deserialize bookmarks : load bookmar buffer to load in the dialog (saved from previous use with SerializeBookmarks())
    const std::string& vBookmarks);                // bookmark buffer to load
```

### C Api

this api was sucessfully tested with CImGui

```cpp
    struct IGFD_Selection_Pair
    {
        char* fileName;
        char* filePathName;
    };

    IMGUIFILEDIALOG_API IGFD_Selection_Pair IGFD_Selection_Pair_Get();  // return an initialized IGFD_Selection_Pair            
    IMGUIFILEDIALOG_API void IGFD_Selection_Pair_DestroyContent(
	    IGFD_Selection_Pair *vSelection_Pair);                          // destroy the content of a IGFD_Selection_Pair
    
    struct IGFD_Selection
    {
        IGFD_Selection_Pair* table;
        size_t count;
    };

    IMGUIFILEDIALOG_API IGFD_Selection IGFD_Selection_Get();            // return an initialized IGFD_Selection
    IMGUIFILEDIALOG_API void IGFD_Selection_DestroyContent(
	    IGFD_Selection* vSelection);                                    // destroy the content of a IGFD_Selection

    // constructor / destructor
    IMGUIFILEDIALOG_API ImGuiFileDialog* IGFD_Create(void);             // create the filedialog context
    IMGUIFILEDIALOG_API void IGFD_Destroy(ImGuiFileDialog *vContext);   // destroy the filedialog context

    typedef void (*IGFD_PaneFun)(const char*, void*, bool*);            // callback fucntion for display the pane
    
    IMGUIFILEDIALOG_API void IGFD_OpenDialog(                           // open a standard dialog
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vPath,                                              // path
        const char* vFileName,                                          // defaut file name
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 
    
    IMGUIFILEDIALOG_API void IGFD_OpenDialog2(                          // open a standard dialog
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vFilePathName,                                      // defaut file path name (path and filename witl be extracted from it)
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API void IGFD_OpenPaneDialog(                       // open a standard dialog with pane
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vPath,                                              // path
        const char* vFileName,                                          // defaut file name
        const IGFD_PaneFun vSidePane,                                   // side pane
        const float vSidePaneWidth,                                     // side pane base width
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API void IGFD_OpenPaneDialog2(                      // open a standard dialog with pane
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vFilePathName,                                      // defaut file name (path and filename witl be extracted from it)
        const IGFD_PaneFun vSidePane,                                   // side pane
        const float vSidePaneWidth,                                     // side pane base width
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API void IGFD_OpenModal(                            // open a modal dialog
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vPath,                                              // path
        const char* vFileName,                                          // defaut file name
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API void IGFD_OpenModal2(                           // open a modal dialog
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vFilePathName,                                      // defaut file name (path and filename witl be extracted from it)
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API void IGFD_OpenPaneModal(                        // open a modal dialog with pane
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vPath,                                              // path
        const char* vFileName,                                          // defaut file name
        const IGFD_PaneFun vSidePane,                                   // side pane
        const float vSidePaneWidth,                                     // side pane base width
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API void IGFD_OpenPaneModal2(                       // open a modal dialog with pane
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog
        const char* vTitle,                                             // title
        const char* vFilters,                                           // filters/filter collections. set it to null for directory mode 
        const char* vFilePathName,                                      // defaut file name (path and filename witl be extracted from it)
        const IGFD_PaneFun vSidePane,                                   // side pane
        const float vSidePaneWidth,                                     // side pane base width
        const int vCountSelectionMax,                                   // count selection max
        void* vUserDatas,                                               // user datas (can be retrieved in pane)
        ImGuiFileDialogFlags vFlags);                                   // ImGuiFileDialogFlags 

    IMGUIFILEDIALOG_API bool IGFD_DisplayDialog(                        // Display the dialog
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context
        const char* vKey,                                               // key dialog to display (if not the same key as defined by OpenDialog/Modal => no opening)
        ImGuiWindowFlags vFlags,                                        // ImGuiWindowFlags
        ImVec2 vMinSize,                                                // mininmal size contraint for the ImGuiWindow
        ImVec2 vMaxSize);                                               // maximal size contraint for the ImGuiWindow

    IMGUIFILEDIALOG_API void IGFD_CloseDialog(                          // Close the dialog
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context            

    IMGUIFILEDIALOG_API bool IGFD_IsOk(                                 // true => Dialog Closed with Ok result / false : Dialog closed with cancel result
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context        

    IMGUIFILEDIALOG_API bool IGFD_WasKeyOpenedThisFrame(                // say if the dialog key was already opened this frame
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context        
        const char* vKey);

    IMGUIFILEDIALOG_API bool IGFD_WasOpenedThisFrame(                   // say if the dialog was already opened this frame
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context    

    IMGUIFILEDIALOG_API bool IGFD_IsKeyOpened(                          // say if the dialog key is opened
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context        
        const char* vCurrentOpenedKey);                                 // the dialog key

    IMGUIFILEDIALOG_API bool IGFD_IsOpened(                             // say if the dialog is opened somewhere    
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context        
    
    IMGUIFILEDIALOG_API IGFD_Selection IGFD_GetSelection(               // Open File behavior : will return selection via a map<FileName, FilePathName>
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context        

    IMGUIFILEDIALOG_API char* IGFD_GetFilePathName(                     // Save File behavior : will always return the content of the field with current filter extention and current path
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context                

    IMGUIFILEDIALOG_API char* IGFD_GetCurrentFileName(                  // Save File behavior : will always return the content of the field with current filter extention
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context                

    IMGUIFILEDIALOG_API char* IGFD_GetCurrentPath(                      // will return current path
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context                    

    IMGUIFILEDIALOG_API char* IGFD_GetCurrentFilter(                    // will return selected filter
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context                        

    IMGUIFILEDIALOG_API void* IGFD_GetUserDatas(                        // will return user datas send with Open Dialog/Modal
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context                                            

    IMGUIFILEDIALOG_API void IGFD_SetExtentionInfos(                    // SetExtention datas for have custom display of particular file type
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context 
        const char* vFilter,                                            // extention filter to tune
        ImVec4 vColor,                                                  // wanted color for the display of the file with extention filter
        const char* vIconText);                                         // wanted text or icon of the file with extention filter (can be sued with font icon)

    IMGUIFILEDIALOG_API void IGFD_SetExtentionInfos2(                   // SetExtention datas for have custom display of particular file type
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context 
        const char* vFilter,                                            // extention filter to tune
        float vR, float vG, float vB, float vA,                         // wanted color channels RGBA for the display of the file with extention filter
        const char* vIconText);                                         // wanted text or icon of the file with extention filter (can be sued with font icon)

    IMGUIFILEDIALOG_API bool IGFD_GetExtentionInfos(
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context 
        const char* vFilter,                                            // extention filter (same as used in SetExtentionInfos)
        ImVec4* vOutColor,                                              // color to retrieve
        char** vOutIconText);                                           // icon or text to retrieve

    IMGUIFILEDIALOG_API void IGFD_ClearExtentionInfos(                  // clear extentions setttings
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context

feature : USE_EXPLORATION_BY_KEYS
    IMGUIFILEDIALOG_API void IGFD_SetFlashingAttenuationInSeconds(      // set the flashing time of the line in file list when use exploration keys
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context 
        float vAttenValue);                                             // set the attenuation (from flashed to not flashed) in seconds

feature : USE_BOOKMARK
    IMGUIFILEDIALOG_API char* IGFD_SerializeBookmarks(                  // serialize bookmarks : return bookmark buffer to save in a file
        ImGuiFileDialog* vContext);                                     // ImGuiFileDialog context

    IMGUIFILEDIALOG_API void IGFD_DeserializeBookmarks(                 // deserialize bookmarks : load bookmar buffer to load in the dialog (saved from previous use with SerializeBookmarks())
        ImGuiFileDialog* vContext,                                      // ImGuiFileDialog context 
        const char* vBookmarks);                                        // bookmark buffer to load 
```
