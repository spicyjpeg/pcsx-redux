/***************************************************************************
 *   Copyright (C) 2022 PCSX-Redux authors                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include <algorithm>
#include <cstdlib>
#include "core/sio.h"
#include "core/system.h"
#include "fmt/format.h"
#include "gui/widgets/memcard_manager.h"

PCSX::Widgets::MemcardManager::MemcardManager() {
    m_memoryEditor.OptShowDataPreview = true;
    m_memoryEditor.OptUpperCaseHex = false;
}

void PCSX::Widgets::MemcardManager::init() {
    // Initialize the OpenGL textures used for the icon images
    // This can't happen in the constructor cause our context won't be ready yet
    glGenTextures(15, m_iconTextures);
    for (int i = 0; i < 15; i++) {
        glBindTexture(GL_TEXTURE_2D, m_iconTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB5_A1, 16, 16);
    }
}

bool PCSX::Widgets::MemcardManager::draw(const char* title) {
    bool changed = false;
    Actions action = Actions::None;
    int selectedBlock;

    ImGui::SetNextWindowPos(ImVec2(600, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, &m_show)) {
        ImGui::End();
        return false;
    }

    // Insert or remove memory cards. Send a SIO IRQ to the emulator if this happens as well.
    if (ImGui::Checkbox(_("Memory Card 1 inserted"),
                        &g_emulator->settings.get<Emulator::SettingMcd1Inserted>().value)) {
        g_emulator->m_sio->interrupt();
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox(_("Memory Card 2 inserted"),
                        &g_emulator->settings.get<Emulator::SettingMcd2Inserted>().value)) {
        g_emulator->m_sio->interrupt();
        changed = true;
    }
    ImGui::Checkbox(_("Show memory card contents"), &m_showMemoryEditor);
    ImGui::SameLine();
    if (ImGui::Button(_("Save memory card to file"))) {
        g_emulator->m_sio->SaveMcd(m_selectedCard);
    }
    ImGui::SliderInt("Icon size", &m_iconSize, 16, 512);

    static const char* cardNames[] = {_("Memory card 1"), _("Memory card 2")};
    // Code below is slightly odd because m_selectedCart is 1-indexed while arrays are 0-indexed
    if (ImGui::BeginCombo(_("Card"), cardNames[m_selectedCard - 1])) {
        for (unsigned i = 0; i < 2; i++) {
            if (ImGui::Selectable(cardNames[i], m_selectedCard == i + 1)) {
                m_selectedCard = i + 1;
            }
        }
        ImGui::EndCombo();
    }

    static constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                                  ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
                                                  ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
    PCSX::SIO::McdBlock block; // The current memory card block we're looking into

    if (ImGui::BeginTable("Memory card information", 4, flags)) {
        ImGui::TableSetupColumn("Block number");
        ImGui::TableSetupColumn("Icon");
        ImGui::TableSetupColumn("Title");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (auto i = 1; i < 16; i++) {
            g_emulator->m_sio->GetMcdBlockInfo(m_selectedCard, i, &block);
            int currentFrame = 0; // 1st frame = 0, 2nd frame = 1, 3rd frame = 2
            const auto animationFrames = block.IconCount;

            // Check if we should display the 3rd frame, then check if we should display the 2nd one
            if (m_frameCount >= 40 && animationFrames == 3) {
                currentFrame = 2;
            } else if (m_frameCount >= 20 && animationFrames >= 2) {
                currentFrame = 1;
            }

            // Pointer to the current frame. Skip 16x16 pixels for each frame
            const auto icon = block.Icon + (currentFrame * 16 * 16);
            bool selected = i == 3;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", i);
            ImGui::TableSetColumnIndex(1);
            drawIcon(i, icon);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text(block.Title);
            ImGui::TableSetColumnIndex(3);

            // We have to suffix the action button names with ##number because Imgui
            // can't handle multiple buttons with the same name well
            auto buttonName = fmt::format(_("Format##{}"), i);
            if (ImGui::SmallButton(buttonName.c_str())) {
                g_emulator->m_sio->FormatMcdBlock(m_selectedCard, i);
            }
            ImGui::SameLine();
            
            buttonName = fmt::format(_("Copy##{}"), i);
            if (ImGui::SmallButton(buttonName.c_str())) {
                action = Actions::Copy;
                selectedBlock = i;
                m_pendingAction.popupText = fmt::format("Choose block to copy block {} to", selectedBlock);
            }
            ImGui::SameLine();
            
            buttonName = fmt::format(_("Move##{}"), i);
            if (ImGui::SmallButton(buttonName.c_str())) {
                action = Actions::Move;
                selectedBlock = i;
                m_pendingAction.popupText = fmt::format("Choose block to move block {} to", selectedBlock);
            }
            ImGui::SameLine();

            buttonName = fmt::format(_("Swap##{}"), i);
            if (ImGui::SmallButton(buttonName.c_str())) {
                action = Actions::Swap;
                selectedBlock = i;
                m_pendingAction.popupText = fmt::format("Choose block to swap block {} with", selectedBlock);
            }
        }
        ImGui::EndTable();
    }

    if (m_showMemoryEditor) {
        const auto data = g_emulator->m_sio->GetMcdData(m_selectedCard);
        m_memoryEditor.DrawWindow(_("Memory Card Viewer"), data, PCSX::SIO::MCD_SIZE);
    }

    if (action != Actions::None) {
        m_pendingAction.type = action;
        m_pendingAction.targetCard = m_selectedCard; // Default to current card as the target for the action
        m_pendingAction.sourceBlock = selectedBlock;
        ImGui::OpenPopup(m_pendingAction.popupText.c_str());
    }
    
    if (ImGui::BeginPopupModal(m_pendingAction.popupText.c_str())) {
        if (ImGui::InputText(_("Block"), m_pendingAction.textInput, sizeof(m_pendingAction.textInput),
                             ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            performAction();
            ImGui::CloseCurrentPopup();
        } else if (ImGui::Button("Cancel")) {
            m_pendingAction.type = Actions::None; // Cancel action
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    m_frameCount = (m_frameCount + 1) % 60;
    ImGui::End();
    return changed;
}

void PCSX::Widgets::MemcardManager::drawIcon(int blockNumber, uint16_t* icon) {
    const auto texture = m_iconTextures[blockNumber - 1];
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, icon);
    ImGui::Image((void*)texture, ImVec2(m_iconSize, m_iconSize));
}

// Perform the pending memory card action (Move, copy, swap)
void PCSX::Widgets::MemcardManager::performAction() {
    // Data of source and dest cards respectively
    auto data1 = (uint8_t*)g_emulator->m_sio->GetMcdData(m_selectedCard);
    auto data2 = (uint8_t*)g_emulator->m_sio->GetMcdData(m_pendingAction.targetCard);

    const int sourceBlock = m_pendingAction.sourceBlock;
    const int destBlock = std::atoi(m_pendingAction.textInput);
    auto source = data1 + sourceBlock * PCSX::SIO::MCD_BLOCK_SIZE;
    auto dest = data2 + destBlock * PCSX::SIO::MCD_BLOCK_SIZE;

    if (destBlock > 15) { // Invalid block number, do nothing
        m_pendingAction.type = Actions::None;
        return;
    }

    switch (m_pendingAction.type) {
        case Actions::Move: {
            for (auto i = 0; i < PCSX::SIO::MCD_BLOCK_SIZE; i++) {
                dest[i] = source[i];
                source[i] = 0;
            }
        } break;

        case Actions::Copy:
            std::memcpy(dest, source, PCSX::SIO::MCD_BLOCK_SIZE);
            break;

        case Actions::Swap: {
            for (auto i = 0; i < PCSX::SIO::MCD_BLOCK_SIZE; i++) {
                std::swap(dest[i], source[i]);
            }
        }
    }

    m_pendingAction.type = Actions::None; // Cancel action
}
