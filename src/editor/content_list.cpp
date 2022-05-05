#include "common.h"
#include "content_list.h"

ContentList::ContentList(std::filesystem::path const& initialPath) {
    SetPath(initialPath);
}

void ContentList::SetPath(std::filesystem::path const& path) {
    if (m_useCwd) {
        std::filesystem::current_path(std::filesystem::absolute(path));
    } else {
        m_currentPath = std::filesystem::absolute(path);
    }
    m_selectedIndex = -1;
    Refresh();
    if (m_changeDirectoryAction) {
        m_changeDirectoryAction(m_currentPath);
    }
}

std::filesystem::path ContentList::GetPath() const {
    return m_useCwd ? std::filesystem::current_path() : m_currentPath;
}

std::filesystem::path ContentList::GetCurrentSelection() {
    if (m_selectedIndex == -1) {
        return {};
    }

    auto const& entry = m_currentList[m_selectedIndex];
    return entry.m_path;
}

void ContentList::Refresh() {
    m_currentList.clear();
    auto const currentPath = GetPath();
    if (std::filesystem::exists(currentPath)) {
        if (currentPath.has_parent_path() && currentPath.has_relative_path()) {
            ListEntry parentEntry;
            parentEntry.m_type = ListEntryType::Directory;
            parentEntry.m_path = currentPath.parent_path();
            parentEntry.m_displayName = "..";
            m_currentList.push_back(parentEntry);
        }

        try {
            for (auto& entry : std::filesystem::directory_iterator(currentPath)) {
                if (m_displayFilterAction && !m_displayFilterAction(entry.path())) {
                    continue;
                }
                ListEntry listEntry;
                listEntry.m_type = entry.is_directory() ? ListEntryType::Directory : ListEntryType::File;
                listEntry.m_path = entry.path();
                if (m_displayNameGet) {
                    listEntry.m_displayName = m_displayNameGet(listEntry.m_path);
                } else {
                    listEntry.m_displayName = entry.path().filename().string();
                }
                m_currentList.push_back(listEntry);
            }
        } catch (std::exception e) {
        }
    }

    std::sort(std::begin(m_currentList), std::end(m_currentList), [](auto const& a, auto const& b) {
        if (a.m_type == b.m_type) {
            return a.m_displayName < b.m_displayName;
        } else if (a.m_type == ListEntryType::Directory) {
            return true;
        } else {
            return false;
        }
    });
}

void ContentList::Draw() {
    ImGui::PushID(this);
    if (ImGui::BeginListBox("##content_list", ImVec2(-FLT_MIN, -FLT_MIN))) {
        for (int i = 0; i < m_currentList.size(); ++i) {
            auto const& entryInfo = m_currentList[i];
            bool const selected = m_selectedIndex == i;
            if (ImGui::Selectable(entryInfo.m_displayName.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                m_selectedIndex = i;
                if (m_clickAction) {
                    m_clickAction(entryInfo.m_path);
                }
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (entryInfo.m_type == ListEntryType::Directory) {
                        SetPath(entryInfo.m_path);
                        break;
                    } else if (m_doubleClickAction) {
                        m_doubleClickAction(entryInfo.m_path);
                    }
                }
            }

            if (m_perEntryAction) {
                m_perEntryAction(entryInfo.m_path);
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopID();
}
