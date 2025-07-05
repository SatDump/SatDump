#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#ifndef IMGUI_VERSION
#   error "include imgui.h before this header"
#endif

using ImGuiFileBrowserFlags = std::uint32_t;

enum ImGuiFileBrowserFlags_ : std::uint32_t
{
    ImGuiFileBrowserFlags_SelectDirectory = 1 << 0,  // select directory instead of regular file
    ImGuiFileBrowserFlags_EnterNewFilename = 1 << 1,  // allow user to enter new filename when selecting regular file
    ImGuiFileBrowserFlags_NoModal = 1 << 2,  // file browsing window is modal by default. specify this to use a popup window
    ImGuiFileBrowserFlags_NoTitleBar = 1 << 3,  // hide window title bar
    ImGuiFileBrowserFlags_NoStatusBar = 1 << 4,  // hide status bar at the bottom of browsing window
    ImGuiFileBrowserFlags_CloseOnEsc = 1 << 5,  // close file browser when pressing 'ESC'
    ImGuiFileBrowserFlags_CreateNewDir = 1 << 6,  // allow user to create new directory
    ImGuiFileBrowserFlags_MultipleSelection = 1 << 7,  // allow user to select multiple files. this will hide ImGuiFileBrowserFlags_EnterNewFilename
    ImGuiFileBrowserFlags_HideRegularFiles = 1 << 8,  // hide regular files when ImGuiFileBrowserFlags_SelectDirectory is enabled
    ImGuiFileBrowserFlags_ConfirmOnEnter = 1 << 9,  // confirm selection when pressing 'ENTER'
    ImGuiFileBrowserFlags_SkipItemsCausingError = 1 << 10, // when entering a new directory, any error will interrupt the process, causing the file browser to fall back to the working directory.
    // with this flag, if an error is caused by a specific item in the directory, that item will be skipped, allowing the process to continue.
    ImGuiFileBrowserFlags_EditPathString = 1 << 11, // allow user to directly edit the whole path string
};

namespace ImGui
{
    class FileBrowser
    {
    public:

        explicit FileBrowser(
            ImGuiFileBrowserFlags flags = 0,
            std::filesystem::path defaultDirectory = std::filesystem::current_path());

        FileBrowser(const FileBrowser& copyFrom);

        FileBrowser& operator=(const FileBrowser& copyFrom);

        // set the window position (in pixels)
        // default is centered
        void SetWindowPos(int posX, int posY) noexcept;

        // set the window size (in pixels)
        // default is (700, 450)
        void SetWindowSize(int width, int height) noexcept;

        // set the window title text
        void SetTitle(std::string title);

        // open the browsing window
        void Open();

        // close the browsing window
        void Close();

        // the browsing window is opened or not
        bool IsOpened() const noexcept;

        // display the browsing window if opened
        void Display();

        // returns true when there is a selected filename
        bool HasSelected() const noexcept;

        // set current browsing directory
        bool SetDirectory(const std::filesystem::path& dir = std::filesystem::current_path());

        // legacy interface. use SetDirectory instead.
        bool SetPwd(const std::filesystem::path& dir = std::filesystem::current_path())
        {
            return SetDirectory(dir);
        }

        // get current browsing directory
        const std::filesystem::path& GetDirectory() const noexcept;

        // legacy interface. use GetDirectory instead.
        const std::filesystem::path& GetPwd() const noexcept
        {
            return GetDirectory();
        }

        // returns selected filename. make sense only when HasSelected returns true
        // when ImGuiFileBrowserFlags_MultipleSelection is enabled, only one of
        // selected filename will be returned
        std::filesystem::path GetSelected() const;

        // returns all selected filenames.
        // when ImGuiFileBrowserFlags_MultipleSelection is enabled, use this
        // instead of GetSelected
        std::vector<std::filesystem::path> GetMultiSelected() const;

        // set selected filename to empty
        void ClearSelected();

        // (optional) set file type filters. eg. { ".h", ".cpp", ".hpp" }
        // ".*" matches any file types
        void SetTypeFilters(const std::vector<std::string>& typeFilters);

        // set currently applied type filter
        // default value is 0 (the first type filter)
        void SetCurrentTypeFilterIndex(int index);

        // when ImGuiFileBrowserFlags_EnterNewFilename is set
        // this function will pre-fill the input dialog with a filename.
        void SetInputName(std::string_view input);

    private:

        template <class Functor>
        struct ScopeGuard
        {
            ScopeGuard(Functor&& t) : func(std::move(t)) {}

            ~ScopeGuard() { func(); }

        private:

            Functor func;
        };

        struct FileRecord
        {
            bool                  isDir = false;
            std::filesystem::path name;
            std::string           showName;
            std::filesystem::path extension;
        };

        static std::string ToLower(const std::string& s);

        void ToolTip(const std::string_view& s);

        void UpdateFileRecords();

        void SetCurrentDirectoryUncatched(const std::filesystem::path& pwd);

        bool SetCurrentDirectoryInternal(
            const std::filesystem::path& dir,
            const std::filesystem::path& preferredFallback);

        bool IsExtensionMatched(const std::filesystem::path& extension) const;

        void ClearRangeSelectionState();

        static void AssignToArrayStyleString(std::vector<char>& arr, std::string_view content);

        static int ExpandInputBuffer(ImGuiInputTextCallbackData* callbackData);

        // for c++17 compatibility

#if defined(__cpp_lib_char8_t)
        static std::string u8StrToStr(std::u8string s);
#endif
        static std::string u8StrToStr(std::string s);

        static std::filesystem::path u8StrToPath(const char* str);

        int width_;
        int height_;
        int posX_;
        int posY_;
        ImGuiFileBrowserFlags flags_;
        std::filesystem::path defaultDirectory_;

        std::string title_;
        std::string openLabel_;

        bool shouldOpen_;
        bool shouldClose_;
        bool isOpened_;
        bool isOk_;
        bool isPosSet_;

        std::string statusStr_;

        std::vector<std::string> typeFilters_;
        unsigned int             typeFilterIndex_;
        bool                     hasAllFilter_;

        std::filesystem::path   currentDirectory_;
        std::vector<FileRecord> fileRecords_;

        unsigned int                    rangeSelectionStart_; // enable range selection when shift is pressed
        std::set<std::filesystem::path> selectedFilenames_;

        std::string       openNewDirLabel_;
        std::vector<char> newDirNameBuffer_;
        std::vector<char> inputNameBuffer_;
        std::string       customizedInputName_;

        bool              editDir_;
        bool              setFocusToEditDir_;
        std::vector<char> currDirBuffer_;
    };
} // namespace ImGui

inline ImGui::FileBrowser::FileBrowser(ImGuiFileBrowserFlags flags, std::filesystem::path defaultDirectory)
    : width_(700)
    , height_(450)
    , posX_(0)
    , posY_(0)
    , flags_(flags)
    , defaultDirectory_(std::move(defaultDirectory))
    , shouldOpen_(false)
    , shouldClose_(false)
    , isOpened_(false)
    , isOk_(false)
    , isPosSet_(false)
    , rangeSelectionStart_(0)
    , editDir_(false)
    , setFocusToEditDir_(false)
{
    assert(!((flags_ & ImGuiFileBrowserFlags_SelectDirectory) && (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)) &&
        "'EnterNewFilename' doesn't work when 'SelectDirectory' is enabled");
    if (flags_ & ImGuiFileBrowserFlags_CreateNewDir)
    {
        newDirNameBuffer_.resize(32, '\0');
    }

    SetTitle("file browser");
    SetDirectory(defaultDirectory_);

    typeFilters_.clear();
    typeFilterIndex_ = 0;
    hasAllFilter_ = false;
}

inline ImGui::FileBrowser::FileBrowser(const FileBrowser& copyFrom)
    : FileBrowser()
{
    *this = copyFrom;
}

inline ImGui::FileBrowser& ImGui::FileBrowser::operator=(
    const FileBrowser& copyFrom)
{
    width_ = copyFrom.width_;
    height_ = copyFrom.height_;

    posX_ = copyFrom.posX_;
    posY_ = copyFrom.posY_;

    flags_ = copyFrom.flags_;
    SetTitle(copyFrom.title_);

    shouldOpen_ = copyFrom.shouldOpen_;
    shouldClose_ = copyFrom.shouldClose_;
    isOpened_ = copyFrom.isOpened_;
    isOk_ = copyFrom.isOk_;
    isPosSet_ = copyFrom.isPosSet_;

    statusStr_ = "";

    typeFilters_ = copyFrom.typeFilters_;
    typeFilterIndex_ = copyFrom.typeFilterIndex_;
    hasAllFilter_ = copyFrom.hasAllFilter_;

    selectedFilenames_ = copyFrom.selectedFilenames_;
    rangeSelectionStart_ = copyFrom.rangeSelectionStart_;

    currentDirectory_ = copyFrom.currentDirectory_;
    fileRecords_ = copyFrom.fileRecords_;

    openNewDirLabel_ = copyFrom.openNewDirLabel_;
    newDirNameBuffer_ = copyFrom.newDirNameBuffer_;
    inputNameBuffer_ = copyFrom.inputNameBuffer_;
    customizedInputName_ = copyFrom.customizedInputName_;

    editDir_ = copyFrom.editDir_;
    currDirBuffer_ = copyFrom.currDirBuffer_;

    return *this;
}

inline void ImGui::FileBrowser::SetWindowPos(int posX, int posY) noexcept
{
    posX_ = posX;
    posY_ = posY;
    isPosSet_ = true;
}

inline void ImGui::FileBrowser::SetWindowSize(int width, int height) noexcept
{
    assert(width > 0 && height > 0);
    width_ = width;
    height_ = height;
}

inline void ImGui::FileBrowser::SetTitle(std::string title)
{
    title_ = std::move(title);

    const std::string thisPtrStr = std::to_string(reinterpret_cast<size_t>(this));
    openLabel_ = title_ + "##filebrowser_" + thisPtrStr;
    openNewDirLabel_ = "new dir##new_dir_" + thisPtrStr;
}

inline void ImGui::FileBrowser::Open()
{
    UpdateFileRecords();
    ClearSelected();
    statusStr_ = std::string();
    shouldOpen_ = true;
    shouldClose_ = false;
    if ((flags_ & ImGuiFileBrowserFlags_EnterNewFilename) && !customizedInputName_.empty())
    {
        AssignToArrayStyleString(inputNameBuffer_, customizedInputName_);
        selectedFilenames_ = { u8StrToPath(inputNameBuffer_.data()) };
    }
}

inline void ImGui::FileBrowser::Close()
{
    ClearSelected();
    statusStr_ = std::string();
    shouldClose_ = true;
    shouldOpen_ = false;
}

inline bool ImGui::FileBrowser::IsOpened() const noexcept
{
    return isOpened_;
}

inline void ImGui::FileBrowser::Display()
{
    PushID(this);
    ScopeGuard exitThis([this]
        {
            shouldOpen_ = false;
            shouldClose_ = false;
            PopID();
        });

    if (shouldOpen_)
    {
        OpenPopup(openLabel_.c_str());
    }
    isOpened_ = false;

    // open the popup window

    if (shouldOpen_ && (flags_ & ImGuiFileBrowserFlags_NoModal))
    {
        if (isPosSet_)
        {
            SetNextWindowPos(ImVec2(static_cast<float>(posX_), static_cast<float>(posY_)));
        }
        SetNextWindowSize(ImVec2(static_cast<float>(width_), static_cast<float>(height_)));
    }
    else
    {
        if (isPosSet_)
        {
            SetNextWindowPos(ImVec2(static_cast<float>(posX_), static_cast<float>(posY_)), ImGuiCond_FirstUseEver);
        }
        SetNextWindowSize(ImVec2(static_cast<float>(width_), static_cast<float>(height_)), ImGuiCond_FirstUseEver);
    }
    if (flags_ & ImGuiFileBrowserFlags_NoModal)
    {
        if (!BeginPopup(openLabel_.c_str()))
        {
            return;
        }
    }
    else if (!BeginPopupModal(openLabel_.c_str(), nullptr,
        flags_ & ImGuiFileBrowserFlags_NoTitleBar ? ImGuiWindowFlags_NoTitleBar : 0))
    {
        return;
    }

    isOpened_ = true;
    ScopeGuard endPopup([] { EndPopup(); });

    std::filesystem::path newDir; bool shouldSetNewDir = false;

    if (editDir_)
    {
        if (setFocusToEditDir_) // Automatically set the text box to be focused on appearing
        {
            SetKeyboardFocusHere();
        }

        PushItemWidth(-1);
        const bool enter = InputText(
            "##directory", currDirBuffer_.data(), currDirBuffer_.size(),
            ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll,
            ExpandInputBuffer, &currDirBuffer_);
        PopItemWidth();

        if (!IsItemActive() && !setFocusToEditDir_)
        {
            editDir_ = false;
        }
        setFocusToEditDir_ = false;

        if (enter)
        {
            std::filesystem::path enteredDir = u8StrToPath(currDirBuffer_.data());
            if (is_directory(enteredDir))
            {
                newDir = std::move(enteredDir);
                shouldSetNewDir = true;
            }
            else if (is_directory(enteredDir.parent_path()))
            {
                newDir = enteredDir.parent_path();
                shouldSetNewDir = true;
            }
            else
            {
                statusStr_ = "[" + std::string(currDirBuffer_.data()) + "] is not a valid directory";
            }
        }
    }
    else
    {
        // display elements in pwd
        int secIdx = 0, newDirLastSecIdx = -1;
        for (const auto& sec : currentDirectory_)
        {
            PushID(secIdx);
            if (secIdx > 0)
            {
                SameLine();
            }
            if (SmallButton(u8StrToStr(sec.u8string()).c_str()))
            {
                newDirLastSecIdx = secIdx;
            }
            PopID();

            ++secIdx;
        }

        if (newDirLastSecIdx >= 0)
        {
            int i = 0;
            std::filesystem::path dstDir;
            for (const auto& sec : currentDirectory_)
            {
                if (i++ > newDirLastSecIdx)
                {
                    break;
                }
                dstDir /= sec;
            }

#ifdef _WIN32
            if (newDirLastSecIdx == 0)
            {
                dstDir /= "\\";
            }
#endif

            SetDirectory(dstDir);
        }

        if (flags_ & ImGuiFileBrowserFlags_EditPathString)
        {
            SameLine();

            if (SmallButton("#"))
            {
                const auto currDirStr = u8StrToStr(currentDirectory_.u8string());
                currDirBuffer_.resize(currDirStr.size() + 1);
                std::memcpy(currDirBuffer_.data(), currDirStr.data(), currDirStr.size());
                currDirBuffer_.back() = '\0';

                editDir_ = true;
                setFocusToEditDir_ = true;
            }
            else
            {
                ToolTip("Edit the current path");
            }
        }
    }

    SameLine();
    if (SmallButton("*"))
    {
        UpdateFileRecords();

        std::set<std::filesystem::path> newSelectedFilenames;
        for (auto& name : selectedFilenames_)
        {
            const auto it = std::find_if(
                fileRecords_.begin(), fileRecords_.end(), [&](const FileRecord& record)
                {
                    return name == record.name;
                });
            if (it != fileRecords_.end())
            {
                newSelectedFilenames.insert(name);
            }
        }

        if ((flags_ & ImGuiFileBrowserFlags_EnterNewFilename) && !inputNameBuffer_.empty() && inputNameBuffer_[0])
        {
            newSelectedFilenames.insert(u8StrToPath(inputNameBuffer_.data()));
        }
    }
    else
    {
        ToolTip("Refresh");
    }

    bool focusOnInputText = false;
    if (flags_ & ImGuiFileBrowserFlags_CreateNewDir)
    {
        SameLine();
        if (SmallButton("+"))
        {
            OpenPopup(openNewDirLabel_.c_str());
            newDirNameBuffer_[0] = '\0';
        }
        else
        {
            ToolTip("Create a new directory");
        }

        if (BeginPopup(openNewDirLabel_.c_str()))
        {
            ScopeGuard endNewDirPopup([] { EndPopup(); });

            InputText(
                "name", newDirNameBuffer_.data(), newDirNameBuffer_.size(),
                ImGuiInputTextFlags_CallbackResize, ExpandInputBuffer, &newDirNameBuffer_);
            focusOnInputText |= IsItemFocused();
            SameLine();

            if (Button("ok") && newDirNameBuffer_[0] != '\0')
            {
                ScopeGuard closeNewDirPopup([] { CloseCurrentPopup(); });
                if (create_directory(currentDirectory_ / u8StrToPath(newDirNameBuffer_.data())))
                {
                    UpdateFileRecords();
                }
                else
                {
                    statusStr_ = "failed to create " + std::string(newDirNameBuffer_.data());
                }
            }
        }
    }

    // browse files in a child window

    float reserveHeight = GetFrameHeightWithSpacing();
    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)
    {
        reserveHeight += GetFrameHeightWithSpacing();
    }

    {
        BeginChild("ch", ImVec2(0, -reserveHeight), true,
            (flags_ & ImGuiFileBrowserFlags_NoModal) ? ImGuiWindowFlags_AlwaysHorizontalScrollbar : 0);
        ScopeGuard endChild([] { EndChild(); });

        const bool shouldHideRegularFiles =
            (flags_ & ImGuiFileBrowserFlags_HideRegularFiles) && (flags_ & ImGuiFileBrowserFlags_SelectDirectory);

        for (unsigned int rscIndex = 0; rscIndex < fileRecords_.size(); ++rscIndex)
        {
            const auto& rsc = fileRecords_[rscIndex];
            if (!rsc.isDir && shouldHideRegularFiles)
            {
                continue;
            }
            if (!rsc.isDir && !IsExtensionMatched(rsc.extension))
            {
                continue;
            }
            if (!rsc.name.empty() && rsc.name.c_str()[0] == '$')
            {
                continue;
            }

            const bool selected = selectedFilenames_.find(rsc.name) != selectedFilenames_.end();

#if IMGUI_VERSION_NUM >= 19100
            const ImGuiSelectableFlags selectableFlag = ImGuiSelectableFlags_NoAutoClosePopups;
#else
            const ImGuiSelectableFlags selectableFlag = ImGuiSelectableFlags_DontClosePopups;
#endif

            if (Selectable(rsc.showName.c_str(), selected, selectableFlag))
            {
                const bool wantDir = flags_ & ImGuiFileBrowserFlags_SelectDirectory;
                const bool canSelect = rsc.name != ".." && rsc.isDir == wantDir;
                const bool rangeSelect =
                    canSelect && GetIO().KeyShift &&
                    rangeSelectionStart_ < fileRecords_.size() &&
                    (flags_ & ImGuiFileBrowserFlags_MultipleSelection) &&
                    IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
                const bool multiSelect =
                    !rangeSelect && GetIO().KeyCtrl &&
                    (flags_ & ImGuiFileBrowserFlags_MultipleSelection) &&
                    IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

                if (rangeSelect)
                {
                    const unsigned int first = (std::min)(rangeSelectionStart_, rscIndex);
                    const unsigned int last = (std::max)(rangeSelectionStart_, rscIndex);
                    selectedFilenames_.clear();
                    for (unsigned int i = first; i <= last; ++i)
                    {
                        if (fileRecords_[i].isDir != wantDir)
                        {
                            continue;
                        }
                        if (!wantDir && !IsExtensionMatched(fileRecords_[i].extension))
                        {
                            continue;
                        }
                        selectedFilenames_.insert(fileRecords_[i].name);
                    }
                }
                else if (selected)
                {
                    if (!multiSelect)
                    {
                        selectedFilenames_ = { rsc.name };
                        rangeSelectionStart_ = rscIndex;
                    }
                    else
                    {
                        selectedFilenames_.erase(rsc.name);
                    }
                    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)
                    {
                        AssignToArrayStyleString(inputNameBuffer_, "");
                    }
                }
                else if (canSelect)
                {
                    if (multiSelect)
                    {
                        selectedFilenames_.insert(rsc.name);
                    }
                    else
                    {
                        selectedFilenames_ = { rsc.name };
                    }
                    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)
                    {
                        const auto rscName = u8StrToStr(rsc.name.u8string());
                        AssignToArrayStyleString(inputNameBuffer_, rscName);
                    }
                    rangeSelectionStart_ = rscIndex;
                }
            }

            if (IsMouseDoubleClicked(ImGuiMouseButton_Left) && IsItemHovered(ImGuiHoveredFlags_None))
            {
                if (rsc.isDir)
                {
                    shouldSetNewDir = true;
                    newDir = (rsc.name != "..") ? (currentDirectory_ / rsc.name) : currentDirectory_.parent_path();
                }
                else if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory))
                {
                    selectedFilenames_ = { rsc.name };
                    isOk_ = true;
                    CloseCurrentPopup();
                }
            }
            else if (IsKeyPressed(ImGuiKey_GamepadFaceDown) && IsItemHovered())
            {
                if (rsc.isDir)
                {
                    shouldSetNewDir = true;
                    newDir = (rsc.name != "..") ? (currentDirectory_ / rsc.name) : currentDirectory_.parent_path();
                    SetKeyboardFocusHere(-1);
                }
                else if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory))
                {
                    selectedFilenames_ = { rsc.name };
                    isOk_ = true;
                    CloseCurrentPopup();
                }
            }
        }
    }

    if (shouldSetNewDir)
    {
        SetDirectory(newDir);
    }

    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename)
    {
        PushID(this);
        ScopeGuard popTextID([] { PopID(); });

        if (inputNameBuffer_.empty())
        {
            inputNameBuffer_.resize(1, '\0');
        }

        PushItemWidth(-1);
        if (InputText(
            "", inputNameBuffer_.data(), inputNameBuffer_.size(),
            ImGuiInputTextFlags_CallbackResize, ExpandInputBuffer, &inputNameBuffer_) && inputNameBuffer_[0] != '\0')
        {
            selectedFilenames_ = { u8StrToPath(inputNameBuffer_.data()) };
        }
        focusOnInputText |= IsItemFocused();
        PopItemWidth();
    }

    if (!focusOnInputText && !editDir_)
    {
        const bool selectAll = (flags_ & ImGuiFileBrowserFlags_MultipleSelection) &&
            IsKeyPressed(ImGuiKey_A) && (IsKeyDown(ImGuiKey_LeftCtrl) ||
                IsKeyDown(ImGuiKey_RightCtrl));
        if (selectAll)
        {
            const bool needDir = flags_ & ImGuiFileBrowserFlags_SelectDirectory;
            selectedFilenames_.clear();
            for (size_t i = 1; i < fileRecords_.size(); ++i)
            {
                auto& record = fileRecords_[i];
                if (record.isDir == needDir &&
                    (needDir || IsExtensionMatched(record.extension)))
                {
                    selectedFilenames_.insert(record.name);
                }
            }
        }
    }

    const bool isEnterPressed =
        (flags_ & ImGuiFileBrowserFlags_ConfirmOnEnter) &&
        IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        IsKeyPressed(ImGuiKey_Enter);
    if (!(flags_ & ImGuiFileBrowserFlags_SelectDirectory))
    {
        if ((Button(" ok ") || isEnterPressed) && !selectedFilenames_.empty())
        {
            isOk_ = true;
            CloseCurrentPopup();
        }
    }
    else
    {
        if (Button(" ok ") || isEnterPressed)
        {
            isOk_ = true;
            CloseCurrentPopup();
        }
    }

    SameLine();

    const bool shouldClose =
        Button("cancel") || shouldClose_ ||
        ((flags_ & ImGuiFileBrowserFlags_CloseOnEsc) &&
            IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            IsKeyPressed(ImGuiKey_Escape));
    if (shouldClose)
    {
        CloseCurrentPopup();
    }

    if (!statusStr_.empty() && !(flags_ & ImGuiFileBrowserFlags_NoStatusBar))
    {
        SameLine();
        Text("%s", statusStr_.c_str());
    }

    if (!typeFilters_.empty())
    {
        SameLine();
        PushItemWidth(8 * GetFontSize());
        if (BeginCombo(
            "##type_filters", typeFilters_[typeFilterIndex_].c_str()))
        {
            ScopeGuard guard([&] { EndCombo(); });

            for (size_t i = 0; i < typeFilters_.size(); ++i)
            {
                bool selected = i == typeFilterIndex_;
                if (Selectable(typeFilters_[i].c_str(), selected) && !selected)
                {
                    typeFilterIndex_ = static_cast<unsigned int>(i);
                }
            }
        }
        PopItemWidth();
    }
}

inline bool ImGui::FileBrowser::HasSelected() const noexcept
{
    return isOk_;
}

inline bool ImGui::FileBrowser::SetDirectory(const std::filesystem::path& dir)
{
    const std::filesystem::path preferredFallback = this->GetDirectory();
    return SetCurrentDirectoryInternal(dir, preferredFallback);
}

inline const std::filesystem::path& ImGui::FileBrowser::GetDirectory() const noexcept
{
    return currentDirectory_;
}

inline std::filesystem::path ImGui::FileBrowser::GetSelected() const
{
    // when isOk_ is true, selectedFilenames_ may be empty if SelectDirectory
    // is enabled. return pwd in that case.
    if (selectedFilenames_.empty())
    {
        return currentDirectory_;
    }
    return currentDirectory_ / *selectedFilenames_.begin();
}

inline std::vector<std::filesystem::path> ImGui::FileBrowser::GetMultiSelected() const
{
    if (selectedFilenames_.empty())
    {
        return { currentDirectory_ };
    }

    std::vector<std::filesystem::path> ret;
    ret.reserve(selectedFilenames_.size());
    for (auto& s : selectedFilenames_)
    {
        ret.push_back(currentDirectory_ / s);
    }

    return ret;
}

inline void ImGui::FileBrowser::ClearSelected()
{
    selectedFilenames_.clear();
    if ((flags_ & ImGuiFileBrowserFlags_EnterNewFilename))
    {
        AssignToArrayStyleString(inputNameBuffer_, "");
    }
    isOk_ = false;
}

inline void ImGui::FileBrowser::SetTypeFilters(const std::vector<std::string>& _typeFilters)
{
    typeFilters_.clear();

    // remove duplicate filter names due to case unsensitivity on windows

#ifdef _WIN32

    std::vector<std::string> typeFilters;
    for (auto& rawFilter : _typeFilters)
    {
        std::string lowerFilter = ToLower(rawFilter);
        const auto it = std::find(typeFilters.begin(), typeFilters.end(), lowerFilter);
        if (it == typeFilters.end())
        {
            typeFilters.push_back(std::move(lowerFilter));
        }
    }

#else

    auto& typeFilters = _typeFilters;

#endif

    // insert auto-generated filter
    hasAllFilter_ = false;
    if (typeFilters.size() > 1)
    {
        hasAllFilter_ = true;
        std::string allFiltersName = std::string();
        for (size_t i = 0; i < typeFilters.size(); ++i)
        {
            if (typeFilters[i] == std::string_view(".*"))
            {
                hasAllFilter_ = false;
                break;
            }

            if (i > 0)
            {
                allFiltersName += ",";
            }
            allFiltersName += typeFilters[i];
        }

        if (hasAllFilter_)
        {
            typeFilters_.push_back(std::move(allFiltersName));
        }
    }

    std::copy(typeFilters.begin(), typeFilters.end(), std::back_inserter(typeFilters_));
    typeFilterIndex_ = 0;
}

inline void ImGui::FileBrowser::SetCurrentTypeFilterIndex(int index)
{
    typeFilterIndex_ = static_cast<unsigned int>(index);
}

inline void ImGui::FileBrowser::SetInputName(std::string_view input)
{
    assert((flags_ & ImGuiFileBrowserFlags_EnterNewFilename) &&
        "SetInputName can only be called when ImGuiFileBrowserFlags_EnterNewFilename is enabled");
    customizedInputName_ = input;
}

inline std::string ImGui::FileBrowser::ToLower(const std::string& s)
{
    std::string ret = s;
    for (char& c : ret)
    {
        c = static_cast<char>(std::tolower(c));
    }
    return ret;
}

inline void ImGui::FileBrowser::ToolTip(const std::string_view& s)
{
    if (!ImGui::IsItemHovered())
    {
        return;
    }
    ImGui::SetTooltip("%s", s.data());
}

inline void ImGui::FileBrowser::UpdateFileRecords()
{
    fileRecords_ = { FileRecord{ true, "..", "[D] ..", "" } };

    for (auto& p : std::filesystem::directory_iterator(currentDirectory_))
    {
        FileRecord rcd;
        try
        {
            if (p.is_regular_file())
            {
                rcd.isDir = false;
            }
            else if (p.is_directory())
            {
                rcd.isDir = true;
            }
            else
            {
                continue;
            }

            rcd.name = p.path().filename();
            if (rcd.name.empty())
            {
                continue;
            }

            rcd.extension = p.path().filename().extension();
            rcd.showName = (rcd.isDir ? "[D] " : "[F] ") + u8StrToStr(p.path().filename().u8string());
        }
        catch (...)
        {
            if (!(flags_ & ImGuiFileBrowserFlags_SkipItemsCausingError))
            {
                throw;
            }
            continue;
        }
        fileRecords_.push_back(rcd);
    }

    // The default lexicographical order does not meet our sorting requirements.
    // We want [b0, a0, A1] to be sorted into something like [a0, A1, b0] instead of [a0, b0, A1].
    // Therefore, here we compute a custom key for each filename for sorting.
    if (fileRecords_.size() > 2)
    {
        std::vector<std::vector<uint32_t>> keys;
        keys.reserve(fileRecords_.size());
        for (auto& fileRecord : fileRecords_)
        {
            const auto name = u8StrToStr(fileRecord.name.u8string());
            auto& key = keys.emplace_back();
            key.reserve(name.size() + 1);
            key.emplace_back(!fileRecord.isDir);
            for (char c : name)
            {
                if ('A' <= c && c <= 'Z')
                {
                    key.emplace_back(2 * (c + 'a' - 'A') + 1);
                }
                else
                {
                    key.emplace_back(2 * c);
                }
            }
        }

        std::vector<uint32_t> fileRecordRemapIndices;
        fileRecordRemapIndices.reserve(fileRecords_.size());
        for (uint32_t i = 0; i < fileRecords_.size(); ++i)
        {
            fileRecordRemapIndices.push_back(i);
        }

        std::sort(
            fileRecordRemapIndices.begin() + 1, fileRecordRemapIndices.end(), [&](uint32_t li, uint32_t ri)
            {
                return keys[li] < keys[ri];
            });

        std::vector<FileRecord> remappedFileRecords;
        remappedFileRecords.reserve(fileRecords_.size());
        for (const uint32_t index : fileRecordRemapIndices)
        {
            remappedFileRecords.emplace_back(std::move(fileRecords_[index]));
        }

        fileRecords_ = std::move(remappedFileRecords);
    }

    ClearRangeSelectionState();
}

inline void ImGui::FileBrowser::SetCurrentDirectoryUncatched(const std::filesystem::path& pwd)
{
    currentDirectory_ = absolute(pwd);
    UpdateFileRecords();

    bool shouldClearInputNameBuffer = true;

    if ((flags_ & ImGuiFileBrowserFlags_EnterNewFilename) &&
        selectedFilenames_.size() == 1 &&
        !customizedInputName_.empty() &&
        !inputNameBuffer_.empty() &&
        std::strcmp(inputNameBuffer_.data(), customizedInputName_.data()) == 0)
    {
        shouldClearInputNameBuffer = false;
    }

    if (shouldClearInputNameBuffer)
    {
        selectedFilenames_.clear();
        AssignToArrayStyleString(inputNameBuffer_, "");
    }
}

inline bool ImGui::FileBrowser::SetCurrentDirectoryInternal(
    const std::filesystem::path& dir, const std::filesystem::path& preferredFallback)
{
    try
    {
        SetCurrentDirectoryUncatched(dir);
        return true;
    }
    catch (const std::exception& err)
    {
        statusStr_ = std::string("error: ") + err.what();
    }
    catch (...)
    {
        statusStr_ = "unknown error";
    }

    if (preferredFallback != defaultDirectory_)
    {
        try
        {
            SetCurrentDirectoryUncatched(preferredFallback);
        }
        catch (...)
        {
            SetCurrentDirectoryUncatched(defaultDirectory_);
        }
    }
    else
    {
        SetCurrentDirectoryUncatched(defaultDirectory_);
    }

    return false;
}

inline bool ImGui::FileBrowser::IsExtensionMatched(const std::filesystem::path& _extension) const
{
#ifdef _WIN32
    std::filesystem::path extension = ToLower(u8StrToStr(_extension.u8string()));
#else
    auto& extension = _extension;
#endif

    // no type filters
    if (typeFilters_.empty())
    {
        return true;
    }

    // invalid type filter index
    if (static_cast<size_t>(typeFilterIndex_) >= typeFilters_.size())
    {
        return true;
    }

    // all type filters
    if (hasAllFilter_ && typeFilterIndex_ == 0)
    {
        for (size_t i = 1; i < typeFilters_.size(); ++i)
        {
            if (extension == typeFilters_[i])
            {
                return true;
            }
        }
        return false;
    }

    // universal filter
    if (typeFilters_[typeFilterIndex_] == std::string_view(".*"))
    {
        return true;
    }

    // regular filter
    return extension == typeFilters_[typeFilterIndex_];
}

inline void ImGui::FileBrowser::ClearRangeSelectionState()
{
    rangeSelectionStart_ = 9999999;
    const bool dir = flags_ & ImGuiFileBrowserFlags_SelectDirectory;
    for (unsigned int i = 1; i < fileRecords_.size(); ++i)
    {
        if (fileRecords_[i].isDir == dir)
        {
            if (!dir && !IsExtensionMatched(fileRecords_[i].extension))
            {
                continue;
            }
            rangeSelectionStart_ = i;
            break;
        }
    }
}

inline void ImGui::FileBrowser::AssignToArrayStyleString(std::vector<char>& arr, std::string_view content)
{
    if (content.empty())
    {
        if (!arr.empty())
        {
            arr[0] = '\0';
        }
        return;
    }

    if (arr.size() < content.size() + 1)
    {
        arr.resize(content.size() + 1);
    }
    std::memcpy(arr.data(), content.data(), content.size());
    arr[content.size()] = '\0';
}

inline int ImGui::FileBrowser::ExpandInputBuffer(ImGuiInputTextCallbackData* callbackData)
{
    if (callbackData && callbackData->EventFlag & ImGuiInputTextFlags_CallbackResize)
    {
        auto buffer = static_cast<std::vector<char>*>(callbackData->UserData);
        size_t newSize = buffer->size();
        while (newSize < static_cast<size_t>(callbackData->BufSize))
        {
            newSize <<= 1;
        }
        buffer->resize(newSize, '\0');
        callbackData->Buf = buffer->data();
        callbackData->BufDirty = true;
    }
    return 0;
}

#if defined(__cpp_lib_char8_t)
inline std::string ImGui::FileBrowser::u8StrToStr(std::u8string s)
{
    std::string result;
    result.resize(s.length());
    std::memcpy(result.data(), s.data(), s.length());
    return result;
}
#endif

inline std::string ImGui::FileBrowser::u8StrToStr(std::string s)
{
    return s;
}

inline std::filesystem::path ImGui::FileBrowser::u8StrToPath(const char* str)
{
#if defined(__cpp_lib_char8_t)
    // With C++20/23, it's impossible to efficiently convert a `char*` string to a `char8_t*` string without violating
    // the strict aliasing rule. Bad joke!
    const size_t len = std::strlen(str);
    std::u8string u8Str;
    u8Str.resize(len);
    std::memcpy(u8Str.data(), str, len);
    return std::filesystem::path(u8Str);
#else
    // u8path is deprecated in C++20
    return std::filesystem::u8path(str);
#endif
}