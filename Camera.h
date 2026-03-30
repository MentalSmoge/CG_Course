#pragma once
#include <SimpleMath.h>
class Camera 
{
public:
	Camera();
	~Camera() = default;
	// Get/Set world camera position.
	DirectX::XMVECTOR GetPositionXM()const;
	DirectX::XMFLOAT3 GetPosition()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	// Get camera basis vectors.
	DirectX::XMVECTOR GetRightXM()const;
	DirectX::XMFLOAT3 GetRight()const;
	DirectX::XMVECTOR GetUpXM()const;
	DirectX::XMFLOAT3 GetUp()const;
	DirectX::XMVECTOR GetLookXM()const;
	DirectX::XMFLOAT3 GetLook()const;
	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;
	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);
	void SetOrthoLens(float width, float height, float zn, float zf);
	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& up);
	// Get View/Proj matrices.
	DirectX::XMMATRIX View()const;
	DirectX::XMMATRIX Proj()const;
	DirectX::XMMATRIX ViewProj()const;
	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);
	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);
	// After modifying camera position/orientation, call
	// to rebuild the view matrix once per frame.
	void UpdateViewMatrix();// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition; // view space origin
	DirectX::XMFLOAT3 mRight;    // view space x-axis
	DirectX::XMFLOAT3 mUp;
	// view space y-axis
	DirectX::XMFLOAT3 mLook;     // view space z-axis
	// Cache frustum properties.
	float mNearZ;
	float mFarZ;
	float mAspect;
	float mFovY;
	float mNearWindowHeight;
	float mFarWindowHeight;
	// Cache View/Proj matrices.
	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;
};