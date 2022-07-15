#pragma once

#include "Widget.h"
#include <memory>
#include "ColorPicker.h"

namespace PlayGround
{
    class Entity;
    class Transform;
    class Light;
    class Renderable;
    class RigidBody;
    class Collider;
    class Material;
    class Camera;
    class Environment;
    class IComponent;
}

// ��ƼƼ�� ������Ʈ�� �߰��� �� �ְ����ִ� ����
class Properties : public Widget
{
public:
    Properties(Editor* editor);

    void UpdateVisible() override;

    // ���õ� ��ƼƼ, ���׸��� ���� ����
    static void Inspect(const std::weak_ptr<PlayGround::Entity>& entity);
    static void Inspect(const std::weak_ptr<PlayGround::Material>& material);

    static std::weak_ptr<PlayGround::Entity> m_InspectedEntity;
    static std::weak_ptr<PlayGround::Material> m_InspectedMaterial;

private:
    void ShowTransform(PlayGround::Transform* transform) const;
    void ShowLight(PlayGround::Light* light) const;
    void ShowRenderable(PlayGround::Renderable* renderable) const;
    void ShowRigidBody(PlayGround::RigidBody* rigid_body) const;
    void ShowCollider(PlayGround::Collider* collider) const;
    void ShowMaterial(PlayGround::Material* material) const;
    void ShowCamera(PlayGround::Camera* camera) const;
    void ShowEnvironment(PlayGround::Environment* environment) const;

    void ShowAddComponentButton() const;
    void ComponentContextMenu_Add() const;

    std::unique_ptr<ColorPicker> m_MaterialColorPicker;
    std::unique_ptr<ColorPicker> m_LightColorPicker;
    std::unique_ptr<ColorPicker> m_CameraColorPicker;
};

