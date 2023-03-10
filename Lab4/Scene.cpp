#include "Scene.h"

Scene::Scene() : m_animationTime(0.0), m_playAnimation(true), m_isGrabbed(false), m_zoom(8.0f)
{
    m_cameraXRotationMatrix = DirectX::XMMatrixIdentity();
    m_cameraYRotationMatrix = DirectX::XMMatrixIdentity();
    Update(0.0);
}

void Scene::Update(double deltaTime)
{
    if (m_playAnimation)
    {
        m_animationTime += deltaTime * 2 * M_PI * 0.25;
    }
    m_modelTransform = DirectX::XMMatrixRotationAxis({ 0, 1, 0 }, -static_cast<float>(m_animationTime));
    m_modelTransform *= DirectX::XMMatrixTranslation(0.0f, (1.0f + sinf(-static_cast<float>(m_animationTime))) / 4, 0.0f);

    m_cameraTransform = DirectX::XMMatrixTranslation(0, 0, -m_zoom);
    m_cameraTransform *= m_cameraYRotationMatrix;
    if (m_isGrabbed)
    {
        m_cameraTransform *= DirectX::XMMatrixRotationAxis({ 1, 0, 0 }, (m_grabLast.y - m_grabStart.y) / 200.f);
    }
    m_cameraTransform *= m_cameraXRotationMatrix;
    if (m_isGrabbed)
    {
        m_cameraTransform *= DirectX::XMMatrixRotationAxis({ 0, 1, 0 }, (m_grabLast.x - m_grabStart.x) / 200.f);
    }
}

const DirectX::XMMATRIX& Scene::GetModelTransform()
{
    return m_modelTransform;
}

const DirectX::XMMATRIX& Scene::GetCameraTransform()
{
    return m_cameraTransform;
}

void Scene::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
    if (wParam == VK_SPACE)
    {
        m_playAnimation = !m_playAnimation;
    }
}

void Scene::OnKeyUp(WPARAM wParam, LPARAM lParam)
{

}

void Scene::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    if (m_isGrabbed)
    {
        m_grabLast.x = GET_X_LPARAM(lParam);
        m_grabLast.y = GET_Y_LPARAM(lParam);
    }
}


void Scene::OnLMouseDown(WPARAM wParam, LPARAM lParam)
{
    m_grabLast.x = m_grabStart.x = GET_X_LPARAM(lParam);
    m_grabLast.y = m_grabStart.y = GET_Y_LPARAM(lParam);
    m_isGrabbed = true;
}

void Scene::OnLMouseUp(WPARAM wParam, LPARAM lParam)
{
    m_grabLast.x = GET_X_LPARAM(lParam);
    m_grabLast.y =  GET_Y_LPARAM(lParam);
    m_isGrabbed = false;
    m_cameraXRotationMatrix *= DirectX::XMMatrixRotationAxis({ 0, 1, 0 }, (m_grabLast.x - m_grabStart.x) / 200.f);
    m_cameraYRotationMatrix *= DirectX::XMMatrixRotationAxis({ 1, 0, 0 }, (m_grabLast.y - m_grabStart.y) / 200.f);
}

void Scene::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    m_zoom -= GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f / 5.0f;
}