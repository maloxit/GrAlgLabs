#pragma once

#include "framework.h"

class Scene
{
    DirectX::XMMATRIX m_modelTransform;
    DirectX::XMMATRIX m_cameraTransform;

    DirectX::XMMATRIX m_cameraXRotationMatrix;
    DirectX::XMMATRIX m_cameraYRotationMatrix;

    double m_animationTime;
    bool m_playAnimation;
    bool m_isGrabbed;
    float m_zoom;
    POINT m_grabStart;
    POINT m_grabLast;

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
