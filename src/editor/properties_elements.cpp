#include "common.h"
#include "properties_elements.h"

std::unique_ptr<PropertyEditContextBase> m_currentEditContext;

ImGuiID GetCurrentEditFocusID() {
    if (m_currentEditContext) {
        return m_currentEditContext->GetID();
    }
    return 0;
}

void CommitEditContext() {
    if (m_currentEditContext) {
        m_currentEditContext->Commit();
        m_currentEditContext.reset();
    }
}
