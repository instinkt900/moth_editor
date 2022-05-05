#pragma once

class ContentList {
public:
    ContentList(std::filesystem::path const& initialPath);

    void SetPath(std::filesystem::path const& path);
    std::filesystem::path GetPath() const;
    std::filesystem::path GetCurrentSelection();

    void SetDisplayNameAction(std::function<std::string(std::filesystem::path const&)> const& action) { m_displayNameGet = action; }
    void SetClickAction(std::function<void(std::filesystem::path const&)> const& action) { m_clickAction = action; }
    void SetDoubleClickAction(std::function<void(std::filesystem::path const&)> const& action) { m_doubleClickAction = action; }
    void SetChangeDirectoryAction(std::function<void(std::filesystem::path const&)> const& action) { m_changeDirectoryAction = action; }
    void SetPerEntryAction(std::function<void(std::filesystem::path const&)> const& action) { m_perEntryAction = action; }
    void SetDisplayFilter(std::function<bool(std::filesystem::path const&)> const& action) { m_displayFilterAction = action; }

    void Refresh();
    void Draw();

private:
    enum class ListEntryType {
        Directory,
        File
    };

    struct ListEntry {
        ListEntryType m_type;
        std::filesystem::path m_path;
        std::string m_displayName;
    };

    bool m_useCwd = true;
    std::filesystem::path m_currentPath;
    std::vector<ListEntry> m_currentList;
    int m_selectedIndex = -1;

    std::function<std::string(std::filesystem::path const&)> m_displayNameGet;
    std::function<void(std::filesystem::path const&)> m_clickAction;
    std::function<void(std::filesystem::path const&)> m_doubleClickAction;
    std::function<void(std::filesystem::path const&)> m_changeDirectoryAction;
    std::function<bool(std::filesystem::path const&)> m_displayFilterAction;
    std::function<void(std::filesystem::path const&)> m_perEntryAction;
};
