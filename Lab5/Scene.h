#pragma once

#include "framework.h"

class Scene
{
    DirectX::XMMATRIX m_modelTransform;
    DirectX::XMMATRIX m_cameraTransform;

    float m_cameraXRotationAngle = 0.0f;
    float m_cameraYRotationAngle = 0.0f;
    float m_cameraOriginXTranslation = 0.0f;
    float m_cameraOriginZTranslation = 0.0f;

    double m_animationTime = 0.0;
    bool m_playAnimation = true;

    float m_zoom = 8.0f;

    bool m_isGrabbed = false;;
    POINT m_grabStart;
    POINT m_grabLast;

    bool m_isWDown = false;
    bool m_isADown = false;
    bool m_isSDown = false;
    bool m_isDDown = false;
    bool m_isFirstPerson = false;

public:
    Scene();
    void Update(double deltaTime);
    const DirectX::XMMATRIX& GetModelTransform();
    const DirectX::XMMATRIX& GetCameraTransform();

    void OnKeyDown(WPARAM wParam, LPARAM lParam);
    void OnKeyUp(WPARAM wParam, LPARAM lParam);
    void OnMouseMove(WPARAM wParam, LPARAM lParam);
    void OnLMouseDown(WPARAM wParam, LPARAM lParam);
    void OnLMouseUp(WPARAM wParam, LPARAM lParam);
    void OnMouseWheel(WPARAM wParam, LPARAM lParam);
};
