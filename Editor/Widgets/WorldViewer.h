#pragma once

#include "Widget.h"
#include <memory>
#include "../ImGui/Source/imgui_internal.h"

namespace PlayGround
{
	class Entity;
}

// ¿ùµå ºä¾î À§Á¬
class WorldViewer : public Widget
{
public:
	WorldViewer(Editor* editor);

	void UpdateVisible() override;

private:
	void TreeShow();
	void OnTreeBegin();
	void OnTreeEnd();
	void TreeAddEntity(PlayGround::Entity* entity);
	void HandleClicking();
	void EntityHandleDragDrop(PlayGround::Entity* entity_ptr) const;
	void SetSelectedEntity(const std::shared_ptr<PlayGround::Entity>& entity, bool from_editor = true);

	void PopUps();
	void PopUpContextMenu() const;
	void PopUpEntityRename() const;
	void HandleKeyShortcuts();


	static void ActionEntityDelete(const std::shared_ptr<PlayGround::Entity>& entity);
	static PlayGround::Entity* ActionEntityCreateEmpty();
	static void ActionEntityCreateCube();
	static void ActionEntityCreateQuad();
	static void ActionEntityCreateSphere();
	static void ActionEntityCreateCylinder();
	static void ActionEntityCreateCone();
	static void ActionEntityCreateCamera();
	static void ActionEntityCreateLightDirectional();
	static void ActionEntityCreateLightPoint();
	static void ActionEntityCreateLightSpot();
	static void ActionEntityCreateSkybox();

	std::shared_ptr<PlayGround::Entity> m_EmptyEntity;
	ImVec2 m_WorldViewerSize = ImVec2(0.0f, 0.0f);
	bool m_Expand_to_selection = false;
	bool m_Expanded_to_selection = false;
	ImRect m_Selected_entity_rect;
};

