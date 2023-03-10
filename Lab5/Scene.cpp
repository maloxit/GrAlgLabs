#include "Scene.h"

Scene::Scene()
{
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

    m_cameraTransform = DirectX::XMMatrixIdentity();
    if (!m_isFirstPerson)
    {
        m_cameraTransform *= DirectX::XMMatrixTranslation(0, 0, -m_zoom);
    }

    float yAngle = m_cameraYRotationAngle;
    float xAngle = m_cameraXRotationAngle;
    if (m_isGrabbed)
    {
        yAngle += (m_grabLast.y - m_grabStart.y) / 200.f;
        xAngle += (m_grabLast.x - m_grabStart.x) / 200.f;
    }
    yAngle = max(-M_PI / 2, yAngle);
    yAngle = min(M_PI / 2, yAngle);
    m_cameraTransform *= DirectX::XMMatrixRotationAxis({ 1, 0, 0 }, yAngle);
    m_cameraTransform *= DirectX::XMMatrixRotationAxis({ 0, 1, 0 }, xAngle);

    float moveSpeed = 10.0f;

    if (m_isWDown)
    {
        m_cameraOriginXTranslation += moveSpeed * deltaTime * sinf(xAngle);
        m_cameraOriginZTranslation += moveSpeed * deltaTime * cosf(xAngle);
    }
    if (m_isADown)
    {
        m_cameraOriginXTranslation -= moveSpeed * deltaTime * cosf(xAngle);
        m_cameraOriginZTranslation -= moveSpeed * deltaTime * -sinf(xAngle);
    }
    if (m_isSDown)
    {
        m_cameraOriginXTranslation -= moveSpeed * deltaTime * sinf(xAngle);
        m_cameraOriginZTranslation -= moveSpeed * deltaTime * cosf(xAngle);
    }
    if (m_isDDown)
    {
        m_cameraOriginXTranslation += moveSpeed * deltaTime * cosf(xAngle);
        m_cameraOriginZTranslation += moveSpeed * deltaTime * -sinf(xAngle);
    }
    float z = 0.0f;
    if (m_isFirstPerson)
    {
        z = m_zoom / 4.0f;
    }
   
    m_cameraTransform *= DirectX::XMMatrixTranslation(m_cameraOriginXTranslation, z, m_cameraOriginZTranslation);
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
    switch (wParam)
    {
    case VK_SPACE:
        m_playAnimation = !m_playAnimation;
        break;
        m_playAnimation = !m_playAnimation;
        break;
    default:
    {
        char key = MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
        if (key == '\0')
            break;
        switch (key)
        {
        case 'w':
        case 'W':
            m_isWDown = true;
            break;
        case 'a':
        case 'A':
            m_isADown = true;
            break;
        case 's':
        case 'S':
            m_isSDown = true;
            break;
        case 'd':
        case 'D':
            m_isDDown = true;
            break;
        case 'f':
        case 'F':
            m_isFirstPerson = !m_isFirstPerson;
            break;
        default:
            break;
        }
    }
    }
}

void Scene::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    default:
    {
        char key = MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
        if (key == '\0')
            break;
        switch (key)
        {
        case 'w':
        case 'W':
            m_isWDown = false;
            break;
        case 'a':
        case 'A':
            m_isADown = false;
            break;
        case 's':
        case 'S':
            m_isSDown = false;
            break;
        case 'd':
        case 'D':
            m_isDDown = false;
            break;
        default:
            break;
        }
    }
    }
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
    if (!m_isGrabbed)
    {
        m_grabLast.x = m_grabStart.x = GET_X_LPARAM(lParam);
        m_grabLast.y = m_grabStart.y = GET_Y_LPARAM(lParam);
        m_isGrabbed = true;
    }
}

void Scene::OnLMouseUp(WPARAM wParam, LPARAM lParam)
{
    m_grabLast.x = GET_X_LPARAM(lParam);
    m_grabLast.y =  GET_Y_LPARAM(lParam);
    m_isGrabbed = false;
    m_cameraXRotationAngle += (m_grabLast.x - m_grabStart.x) / 200.f;
    m_cameraYRotationAngle += (m_grabLast.y - m_grabStart.y) / 200.f;

    m_cameraYRotationAngle = max(-M_PI / 2, m_cameraYRotationAngle);
    m_cameraYRotationAngle = min(M_PI / 2, m_cameraYRotationAngle);
}

void Scene::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
    m_zoom -= GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f / 5.0f;
    m_zoom = max(m_zoom, 0.0f);
}