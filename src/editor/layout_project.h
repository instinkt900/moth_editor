#pragma once

struct LayoutProject {
    std::string m_layoutRoot;
    std::string m_imageRoot;
    std::string m_loadedPath;
};

inline void to_json(nlohmann::json& j, LayoutProject const& layoutProject) {
    j["layoutRoot"] = layoutProject.m_layoutRoot;
    j["imageRoot"] = layoutProject.m_imageRoot;
}

inline void from_json(nlohmann::json const& j, LayoutProject& layoutProject) {
    j.at("layoutRoot").get_to(layoutProject.m_layoutRoot);
    j.at("imageRoot").get_to(layoutProject.m_imageRoot);
}
